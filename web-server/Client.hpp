#pragma once

#include <asio/ip/tcp.hpp>
#include <array>
#include <memory>
#include <functional>

namespace kweb_server
{
using ClientCallbackType = std::function<void(const std::array<unsigned char, 2048>&, const asio::error_code&, std::size_t, const std::string&)>;

class Client : public std::enable_shared_from_this<Client>
{
    asio::ip::tcp::socket m_socket;
    std::string m_client_id;

    std::array<unsigned char, 2048> m_read_buffer;
    std::array<unsigned char, 2048> m_write_buffer;

    ClientCallbackType m_on_read;
    ClientCallbackType m_on_write;
public:
    Client(const std::string& id, asio::io_context& io_context, ClientCallbackType on_read, ClientCallbackType on_write);
    ~Client();
    asio::ip::tcp::socket& get_socket();
    std::string get_id() const;
    void read();
    void write(const std::array<unsigned char, 2048>& payload);
};

using ClientPtr = std::shared_ptr<Client>;
}   // namespace kweb_server