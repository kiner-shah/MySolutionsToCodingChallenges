#include "LoadBalancer.hpp"
#include <asio/placeholders.hpp>
#include <asio/connect.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "utils.hpp"

namespace kload_balancer
{
LoadBalancer::LoadBalancer(std::string_view ip_address, std::string_view port, std::int_least64_t health_check_period_in_seconds)
    : m_thread_pool{std::make_shared<ThreadPool>()},
    m_signals{m_thread_pool->get_io_context()},
    m_acceptor{m_thread_pool->get_io_context()}, 
    m_ip_address{std::move(ip_address)},
    m_port{std::move(port)},
    m_logger{spdlog::stdout_color_mt("LoadBalancer")},
    m_user_client_manager{std::make_unique<UserClientManager>()},
    m_lb_client_manager{std::make_shared<LbClientManager>()},
    m_health_checker{std::make_unique<HealthChecker>(m_thread_pool, m_lb_client_manager, health_check_period_in_seconds)}
{
    m_logger->set_level(spdlog::level::debug);
    m_logger->set_pattern("%Y-%m-%d %H:%M:%S | %n | %-8l | %v");

    m_signals.add(SIGINT);
    m_signals.add(SIGTERM);

    m_signals.async_wait([this](std::error_code error, int signal_no)
    {
        stop();
    });

    asio::ip::tcp::resolver endpoint_resolver{m_acceptor.get_executor()};
    asio::ip::tcp::endpoint endpoint = *endpoint_resolver.resolve(ip_address, port).begin();
    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address{true});
    m_acceptor.bind(endpoint);
    m_acceptor.listen();

    m_health_checker->start();

    accept();
}

LoadBalancer::~LoadBalancer()
{
    if (!m_is_stopped.load())
    {
        stop();
    }
}

void LoadBalancer::start()
{
    m_logger->info("Starting KLoadBalancer on {}:{}", m_ip_address, m_port);
    m_thread_pool->start();
    m_thread_pool->get_io_context().run();
}

void LoadBalancer::stop()
{
    m_logger->info("Stopping KLoadBalancer on {}:{}", m_ip_address, m_port);
    m_thread_pool->stop();
    m_is_stopped.store(true);
}

void LoadBalancer::add_server(std::string_view ip_address, std::string_view port)
{
    m_health_checker->add_server(std::move(ip_address), std::move(port));
}

void LoadBalancer::accept()
{
    auto user_client_ptr = m_user_client_manager->create_new_user_client(
        m_thread_pool->get_io_context(),
        std::bind(&LoadBalancer::on_client_read_done, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
        std::bind(&LoadBalancer::on_client_write_done, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
    );

    m_acceptor.async_accept(user_client_ptr->get_socket(),
        [this, user_client_ptr](const asio::error_code &error)
        {
            handle_accept(error, user_client_ptr->get_id());
            accept();
        }
    );
}

void LoadBalancer::handle_accept(const asio::error_code &error, std::string user_client_id)
{
    // Check whether the server was stopped by a signal before this
    // completion handler had a chance to run.
    if (!m_acceptor.is_open())
    {
        return;
    }

    m_logger->debug("New connection request [UserClientId {}]", user_client_id);

    auto user_client_ptr = m_user_client_manager->get_user_client(user_client_id);
    if (!user_client_ptr)
    {
        return;
    }
    if (!error)
    {
        user_client_ptr->get_socket().set_option(asio::ip::tcp::no_delay(true));
        m_logger->debug("Waiting for read to complete for from [UserClientId {}, Local endpoint address {}]",
            user_client_id,
            user_client_ptr->get_socket().local_endpoint().address().to_string());
        user_client_ptr->read();
    }
    else
    {
        m_user_client_manager->remove_user_client(user_client_id);
    }
}

void LoadBalancer::on_client_read_done(std::array<unsigned char, 2048> read_data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id)
{
    if (error)
    {
        return;
    }
    m_logger->debug("Received message from [UserClientId {}]:\n{}", user_client_id, std::string{read_data.begin(), read_data.end()});
    // Get next available server
    auto next_available_server = m_health_checker->get_next_available_server();
    if (!next_available_server)
    {
        
        auto user_client_ptr = m_user_client_manager->get_user_client(user_client_id);
        if (!user_client_ptr)
        {
            m_logger->warn("UserClient not found in list {}", user_client_id);
            return;
        }
        // No server available, write error message to the client
        std::array<unsigned char, 2048> buffer = get_service_unavailable_message();
        m_logger->debug("No servers available [UserClientId {}]", user_client_id);
        user_client_ptr->write(buffer);
        return;
    }
    
    m_logger->debug("Connecting to server {}:{} for [UserClientId {}]", next_available_server.value().get_ip(), next_available_server.value().get_port(), user_client_id);
    auto lb_client_ptr = m_lb_client_manager->create_new_lb_client(
        m_thread_pool->get_io_context(),
        next_available_server.value().get_ip(),
        next_available_server.value().get_port(),
        std::bind(&LoadBalancer::on_server_read_done, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5),
        std::bind(&LoadBalancer::on_server_write_done, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)
    );
    // write to the server
    m_logger->debug("Waiting for write to complete for [LbClient {}]", lb_client_ptr->get_id());
    lb_client_ptr->write(read_data, std::move(user_client_id));
}

void LoadBalancer::on_client_write_done(std::array<unsigned char, 2048> write_data, asio::error_code, std::size_t, std::string user_client_id)
{
    m_logger->debug("Sent message to [UserClientId {}]:\n{}", user_client_id, std::string{write_data.begin(), write_data.end()});
    // close connection to client, remove client
    m_logger->debug("Deleting client [UserClientId {}]", user_client_id);
    m_user_client_manager->remove_user_client(user_client_id);
}

void LoadBalancer::on_server_read_done(std::array<unsigned char, 2048> read_data, asio::error_code error, std::size_t bytes_transferred, std::string user_client_id, std::string server_endpoint)
{
    if (error)
    {
        // Something went wrong while getting response from server.
        // This is an internal server error, write the appropriate message.
        m_logger->error("Internal server error: {} {} {} {} {}", error.message(), bytes_transferred, std::string{read_data.begin(), read_data.end()}, user_client_id, server_endpoint);
        read_data = get_internal_server_error_message();
    }

    auto user_client_ptr = m_user_client_manager->get_user_client(user_client_id);
    if (!user_client_ptr)
    {
        m_logger->error("UserClient not found in list {}", user_client_id);
        return;
    }
    m_logger->debug("Waiting for write to complete for [UserClientId {}]", user_client_id);
    user_client_ptr->write(read_data);

    m_logger->debug("Deleting server [LbClient {}] for [UserClientId {}]", server_endpoint, user_client_id);
    m_lb_client_manager->remove_lb_client(server_endpoint);
}

void LoadBalancer::on_server_write_done(std::array<unsigned char, 2048>, asio::error_code error, std::size_t, std::string user_client_id, std::string server_endpoint)
{
    if (error)
    {
        // TODO: is this an internal server error?
        std::array<unsigned char, 2048> buffer;
        for (std::size_t i = 0; i < error.message().size(); i++)
        {
            buffer[i] = error.message().at(i);
        }

        auto user_client_ptr = m_user_client_manager->get_user_client(user_client_id);
        if (!user_client_ptr)
        {
            m_logger->error("UserClient not found in list {}", user_client_id);
            return;
        }
        m_logger->debug("Waiting for write to complete for [UserClientId {}]", user_client_id);
        user_client_ptr->write(buffer);
    }

    auto lb_client_ptr = m_lb_client_manager->get_lb_client(server_endpoint);
    if (!lb_client_ptr)
    {
        m_logger->warn("Server not found in list {}", server_endpoint);
        return;
    }
    m_logger->debug("Waiting for read to complete for [ClientIpPort {}]", server_endpoint);
    lb_client_ptr->read(std::move(user_client_id));
}
} // namespace kload_balancer