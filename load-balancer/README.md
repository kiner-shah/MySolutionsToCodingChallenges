Implement load balancer as per the [challenge description](https://codingchallenges.fyi/challenges/challenge-load-balancer).

## Dependencies
- [Asio 1.30.2](https://think-async.com/Asio/)
- [spdlog 1.15.2](https://github.com/gabime/spdlog/releases/tag/v1.15.2) (used compiled version, not header-only version)

These dependencies should be present in the project root directory.
- spdlog should be compiled into a library and the install folder should be present in the project root directory
- asio should be downloaded and extracted in the project root directory

## Setup for testing
1. Create three folders: `server8080`, `server8081` and `server8082`. Each of these folder will have an `index.html` file with following content:
    ```
    <!DOCTYPE html>
    <html lang="en">
        <head>
            <meta charset="utf-8">
            <title>Index Page</title>
        </head>
        <body>
            Hello from the web server running at on port 8080.
        </body>
    </html>
    ```
    Replace `8080` above with `8081` and `8082` in `server8081` and `server8082` respectively.
2. Then, run the servers in three separate tabs/windows:
    ```
    python -m http.server 8080 --directory server8080
    ```
3. To test for concurrent requests, create a file `urls.txt` with following content:
    ```
    url = "http://localhost:2000"
    url = "http://localhost:2000"
    url = "http://localhost:2000"
    url = "http://localhost:2000"
    url = "http://localhost:2000"
    url = "http://localhost:2000"
    url = "http://localhost:2000"
    url = "http://localhost:2000"
    ```
    Then, use the following command to make concurrent requests:
    ```
    curl --parallel --parallel-immediate --parallel-max 3 --config urls.txt
    ```
    Tweak the maximum parallelisation to see how well your server copes!

## Notes
1. If you get an error like following:
    ```
    FATAL: ThreadSanitizer: unexpected memory mapping 0x5e51ce1ec000-0x5e51ce1fb000
    ```
    This error occurs when ThreadSanitizer (TSan) (a C/C++ data race detector) fails because the memory layout of the program, often due to high-entropy Address Space Layout Randomization (ASLR), conflicts with the shadow memory mapping required by TSan.
    To solve this, try lowering ASLR entropy. For this, you may need to set `vm.mmap_rnd_bits` value to some lower value:
    ```
    sudo sysctl vm.mmap_rnd_bits=28
    ```
    This change is temporary and will reset to original value on reboot.

## Scribble for improvements - new architecture
ThreadPool:
    io_context
    work_guard
    vector<thread> threads

    ThreadPool() - set work guard, create threads and run io context from each
    ~ThreadPool() - reset work guard, stop io context and join threads

UserClientManager:
    vector<UserClientPtr> - reserve to some value, say 20
    logger
    client_counter
    clients_mutex - shared

    UserClientPtr create_new_user_client(io_context, readcallback, writecallback)
    UserClientPtr get_user_client(id)
    remove_user_client(id)

LbClientManager:
    vector<LbClientPtr> - reserve to some value, say 10
    logger
    lb_clients_mutex - shared

    LbClientPtr create_new_lb_client(io_context, ip, port, health_check=false, readcallback, writecallback)
    LbClientPtr get_lb_client(ip_port, health_check=false)
    remove_lb_client(ip_port)

LbClient:
    endpoint
    socket
    for_health_check=false
    
    read_buffer
    write_buffer

    on_read_complete
    on_write_complete
    
    LbClient(ip, port) -> resolves address and connects to it

    read() -> calls async handle_read
    handle_read() -> calls on_read_complete
    write(data) -> calls async handle_write
    handle_write() -> calls on_write_complete

    ~LbClient() -> closes socket

UserClient:
    socket
    client_id

    read_buffer
    write_buffer

    on_read_complete
    on_write_complete

    UserClient() - socket initialization

    read() -> calls async handle_read
    handle_read() -> calls on_read_complete
    write(data) -> calls async handle_write
    handle_write() -> calls on_write_complete

    ~UserClient() -> closes socket

LoadBalancer:
    port
    LbClientManager
    UserClientManager
    ThreadPoolPtr
    acceptor

    LoadBalancer(port) -> stores port, registers signals and call async_wait (call stop() inside), accept()
    add_server() -> adds a new server
    get_next_available_server() -> gets next available server
    
    start() -> create thread pool ptr here

    accept() -> calls handle_accept()
    handle_accept() -> creates a new client with next available client_id, threadpool io_context and calls its read()
    on_client_read_done() -> created a new server with threadpool io_context and calls its write()
    on_client_write_done() -> closes the connection with client
    on_server_read_done() -> calls client write(), closes connection with server
    on_server_write_done() -> calls server read()
    
    stop() -> reset thread pool ptr here

    ~LoadBalancer() -> calls stop()


HealthChecker:
    steady_timer
    const LbClientManager& manager

    HealthChecker(period) -> initializes timer, starts thread
    add_server(ip_address, port) -> adds a new server to vector
    std::optional<ServerDetails> get_next_available_server()
    start() -> calls async_wait
    cancel() -> cancels timer
    perform_health_check() -> connects to servers and sends a GET request
    handle_timer_complete() -> calls perform_health_check(), sets new expiry and calls async_wait
    on_server_write_done() -> calls server read
    on_server_read_done() -> sets availability (if no error), closes server connection
