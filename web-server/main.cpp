#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "WebServer.hpp"

int main()
{
    kweb_server::WebServer server{3000, "../www"};

    server.register_handler(
        kweb_server::HttpMethod::Get,
        "/version",
        [](const kweb_server::HttpRequest& request) -> kweb_server::HttpResponse
        {
            return kweb_server::HttpResponse{kweb_server::HttpStatusCode::Ok, "0.1.0"};
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
