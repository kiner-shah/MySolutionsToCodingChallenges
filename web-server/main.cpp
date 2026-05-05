#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "WebServer.hpp"

int main()
{
    kweb_server::WebServer server{3000};
    std::filesystem::path www_root{"../www"};

    server.register_handler(
        kweb_server::HttpMethod::Get,
        "/",
        [www_root](const kweb_server::HttpRequest& request) -> kweb_server::HttpResponse
        {
            if (request.get_path() != "/")
            {
                return kweb_server::HttpResponse{kweb_server::HttpStatusCode::NotFound, ""};
            }
            std::filesystem::path file_path = www_root / "index.html";
            std::ifstream file_stream(file_path.generic_string());
            if (!file_stream.is_open())
            {
                return kweb_server::HttpResponse{kweb_server::HttpStatusCode::InternalServerError, ""};
            }
            std::string file_contents((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
            return kweb_server::HttpResponse{kweb_server::HttpStatusCode::Ok, file_contents};
        }
    );
    server.register_handler(
        kweb_server::HttpMethod::Get,
        "/index.html",
        [www_root](const kweb_server::HttpRequest& request) -> kweb_server::HttpResponse
        {
            if (request.get_path() != "/index.html")
            {
                return kweb_server::HttpResponse{kweb_server::HttpStatusCode::NotFound, ""};
            }
            std::filesystem::path file_path = www_root / "index.html";
            std::ifstream file_stream(file_path.generic_string());
            if (!file_stream.is_open())
            {
                return kweb_server::HttpResponse{kweb_server::HttpStatusCode::InternalServerError, ""};
            }
            std::string file_contents((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
            return kweb_server::HttpResponse{kweb_server::HttpStatusCode::Ok, file_contents};
        }
    );
    try
    {
        server.start();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
