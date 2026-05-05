#pragma once

#include <asio/ip/tcp.hpp>
#include <memory>
#include <functional>
#include <vector>

namespace kweb_server
{
using ClientReadCallbackType = std::function<void(std::string_view, const asio::error_code&, std::size_t, const std::string&)>;
using ClientWriteCallbackType = std::function<void(const std::vector<unsigned char>&, const asio::error_code&, std::size_t, const std::string&)>;

class Client : public std::enable_shared_from_this<Client>
{
    asio::ip::tcp::socket m_socket;
    std::string m_client_id;

    std::vector<unsigned char> m_read_buffer;
    std::vector<unsigned char> m_write_buffer;
    std::string m_request_buffer;

    ClientReadCallbackType m_on_read;
    ClientWriteCallbackType m_on_write;
public:
    Client(const std::string& id, asio::io_context& io_context, ClientReadCallbackType on_read, ClientWriteCallbackType on_write);
    ~Client();
    asio::ip::tcp::socket& get_socket();
    std::string get_id() const;
    void read();
    void write(const std::string& payload, std::size_t payload_length);
};

using ClientPtr = std::shared_ptr<Client>;
}   // namespace kweb_server