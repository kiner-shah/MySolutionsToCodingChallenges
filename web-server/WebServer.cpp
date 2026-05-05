#include "WebServer.hpp"
#include <charconv>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace
{
void replace_all(std::string& str, const std::string& target, const std::string& replacement)
{
    if (target.empty())
    {
        return;
    }
    size_t start_pos = 0;
    while (true)
    {
        start_pos = str.find(target, start_pos);
        if (start_pos == std::string::npos)
        {
            break;
        }
        str.replace(start_pos, target.length(), replacement);
        start_pos += replacement.length();
    }
}

bool url_decode(std::string& str)
{
    std::size_t percent_pos = 0;
    while (true)
    {
        percent_pos = str.find('%', percent_pos);
        if (percent_pos == std::string::npos)
        {
            break;
        }
        if (percent_pos + 2 >= str.size())
        {
            return false;
        }
        std::string hex_value = str.substr(percent_pos + 1, 2);
        int decoded_char;
        if (std::from_chars(hex_value.data(), hex_value.data() + hex_value.size(), decoded_char, 16).ec != std::errc())
        {
            return false;
        }
        str.replace(percent_pos, 3, 1, static_cast<char>(decoded_char));
    }
    return true;
}

std::optional<kweb_server::HttpRequest> normalize_request(const kweb_server::HttpRequest& request)
{
    std::string path = request.get_path();
    if (!url_decode(path))
    {
        return std::nullopt;
    }
    replace_all(path, "\\", "/");
    
    std::vector<std::string> path_components;
    std::size_t start_pos = 0;
    while (start_pos < path.size())
    {
        std::size_t slash_pos = path.find('/', start_pos);
        if (slash_pos == std::string::npos)
        {
            auto path_component = path.substr(start_pos);
            if (path_component == "..")
            {
                if (path_components.empty())
                {
                    return std::nullopt;
                }
                path_components.pop_back();
                start_pos = slash_pos + 1;
            }
            else
            {
                path_components.push_back(std::move(path_component));
            }
            break;
        }
        auto path_component = path.substr(start_pos, slash_pos - start_pos);
        if (path_component == "." || path_component.empty())
        {
            // Skip
            start_pos = slash_pos + 1;
            continue;
        }
        else if (path_component == "..")
        {
            if (path_components.empty())
            {
                return std::nullopt;
            }
            path_components.pop_back();
            start_pos = slash_pos + 1;
            continue;
        }
        path_components.push_back(std::move(path_component));
        start_pos = slash_pos + 1;   
    }
    path = "/";
    for (const auto& component : path_components)
    {
        path += component + "/";
    }
    if (path.size() > 1)
    {
        path.pop_back(); // Remove trailing slash
    }
    return kweb_server::HttpRequest{request.get_method(), path};
}
}   // namespace

namespace kweb_server
{
void WebServer::handle_client_read(
    const std::array<unsigned char, 2048>& read_buffer,
    const asio::error_code& error,
    std::size_t bytes_transferred,
    const std::string& client_id)
{
    if (error)
    {
        m_logger->error("Encountered error: '{}' while reading from client {}", error.message(), client_id);
        m_logger->debug("Removing client {}", client_id);
        m_client_manager->remove_client(client_id);
        return;
    }

    auto client_ptr = m_client_manager->get_client(client_id);
    if (!client_ptr)
    {
        m_logger->error("Received read event for non-existent client {}", client_id);
        return;
    }
    std::optional<HttpRequest> request = HttpRequest::parse(read_buffer);
    if (!request)
    {
        m_logger->error("Failed to parse request from client {}", client_id);
        HttpResponse response{HttpStatusCode::BadRequest, ""};
        client_ptr->write(response.to_raw_response());
        return;
    }
    auto normalized_request = normalize_request(*request);
    if (!normalized_request)
    {
        m_logger->error("Failed to normalize request [Method: {}, Path: {}] from client {}", static_cast<int>(request->get_method()), request->get_path(), client_id);
        HttpResponse response{HttpStatusCode::BadRequest, ""};
        client_ptr->write(response.to_raw_response());
        return;
    }
    m_logger->info("Received request [Method: {}, Path: {}] from client {}", static_cast<int>(normalized_request->get_method()), normalized_request->get_path(), client_id);

    auto handler_it = m_handlers.find(*normalized_request);
    if (handler_it == m_handlers.end())
    {
        m_logger->error("No handler found for request [Method: {}, Path: {}]", static_cast<int>(request->get_method()), request->get_path());
        HttpResponse response{HttpStatusCode::NotFound, ""};
        client_ptr->write(response.to_raw_response());
    }
    else
    {
        HttpResponse response = handler_it->second(*normalized_request);
        client_ptr->write(response.to_raw_response());
    }
}

void WebServer::handle_client_write(
    const std::array<unsigned char, 2048>& /*write_buffer*/,
    const asio::error_code& error,
    std::size_t /*bytes_transferred*/,
    const std::string& client_id)
{
    if (error)
    {
        m_logger->error("Encountered error: '{}' while writing to client {}", error.message(), client_id);
    }
    m_logger->info("Removing client {}", client_id);
    m_client_manager->remove_client(client_id);
}

void WebServer::handle_accept(const asio::error_code &error, const std::string &client_id)
{
    // Check whether the server was stopped by a signal before this
    // completion handler had a chance to run.
    if (!m_acceptor.is_open())
    {
        return;
    }

    auto client_ptr = m_client_manager->get_client(client_id);
    if (!client_ptr)
    {
        return;
    }
    m_logger->debug("New connection request [ClientId {}]", client_id);
    if (!error)
    {
        client_ptr->get_socket().set_option(asio::ip::tcp::no_delay(true));
        client_ptr->read();
    }
    else
    {
        m_logger->error("Encountered error: '{}' so removing client {}", error.message(),client_id);
        m_client_manager->remove_client(client_id);
    }
}

WebServer::WebServer(unsigned short port)
    : m_thread_pool{std::make_shared<ThreadPool>()},
      m_logger{spdlog::stdout_color_mt("WebServer")},
      m_client_manager{std::make_unique<ClientManager>()},
      m_endpoint_resolver{m_thread_pool->get_io_context()},
      m_signals{m_thread_pool->get_io_context()},
      m_acceptor{m_thread_pool->get_io_context()}
{
    m_logger->set_level(spdlog::level::info);
    m_logger->set_pattern("%Y-%m-%d %H:%M:%S | %n | %-8l | %v");

    m_signals.add(SIGINT);
    m_signals.add(SIGTERM);

    m_signals.async_wait([this](const asio::error_code& error, int signal_number)
    {
        stop();
    });

    m_endpoint_resolver.async_resolve("0.0.0.0", std::to_string(port),
        [this](const asio::error_code& error, asio::ip::tcp::resolver::results_type results)
        {
            if (error)
            {
                m_logger->error("Failed to resolve endpoint: {}", error.message());
                return;
            }

            asio::ip::tcp::endpoint endpoint = *results.begin();
            m_acceptor.open(endpoint.protocol());
            m_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
            m_acceptor.bind(endpoint);
            m_acceptor.listen();
            m_logger->info("Server is listening on port {}", endpoint.port());
            accept();
        }
    );
}

WebServer::~WebServer()
{
    if (!m_is_stopped.load(std::memory_order_relaxed))
    {   
        stop();
    }
}

void WebServer::accept()
{
    auto client_ptr = m_client_manager->create_new_client(
        m_thread_pool->get_io_context(),
        std::bind(&WebServer::handle_client_read, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
        std::bind(&WebServer::handle_client_write, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
    );

    m_acceptor.async_accept(client_ptr->get_socket(),
        [this, client_ptr](const asio::error_code& error)
        {
            handle_accept(error, client_ptr->get_id());
            accept();
        }
    );
}

void WebServer::start()
{
    m_thread_pool->start();
    m_thread_pool->get_io_context().run();
}

void WebServer::stop()
{
    m_thread_pool->stop();
    m_is_stopped.store(true, std::memory_order_relaxed);
}

void WebServer::register_handler(HttpMethod method, const std::string &path, std::function<HttpResponse(const HttpRequest&)> handler)
{
    HttpRequest request{method, path};
    m_handlers.insert_or_assign(request, std::move(handler));
}
}
