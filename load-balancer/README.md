Implement load balancer as per the [challenge description](https://codingchallenges.fyi/challenges/challenge-load-balancer).

## Dependencies
- [Asio 1.30.2](https://think-async.com/Asio/)
- [spdlog 1.15.2](https://github.com/gabime/spdlog/releases/tag/v1.15.2) (used compiled version, not header-only version)

These dependencies should be present in the project root directory.
- spdlog should be compiled into a library and the install folder should be present in the project root directory
- asio should be downloaded and extracted in the project root directory

# Notes
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

## Scribble
Client:
    io_context
    executor_work_guard
    endpoint
    socket
    thread
    
    read_buffer
    write_buffer

    on_read_complete
    on_write_complete
    
    Client(ip, port) -> resolves address and connects to it, calls make_work_guard on io_context, starts io_context in a thread

    read() -> calls async handle_read
    handle_read() -> calls on_read_complete
    write(data) -> calls async handle_write
    handle_write() -> calls on_write_complete

    ~Client() -> resets work_guard and joins thread, closes socket

UserClient:
    io_context
    executor_work_guard
    socket
    client_id

    read_buffer
    write_buffer

    on_read_complete
    on_write_complete

    UserClient() -> calls make_work_guard on io_context and starts io_context in a thread

    read() -> calls async handle_read
    handle_read() -> calls on_read_complete
    write(data) -> calls async handle_write
    handle_write() -> calls on_write_complete

    ~UserClient() -> resets work_guard and joins thread, closes socket

LoadBalancer:
    io_context
    port
    executor_work_guard
    vector<BackendServerPtr>
    vector<ClientPtr>
    acceptor

    LoadBalancer(port) -> stores port, registers signals and call async_wait (call stop() inside), accept()
    add_server() -> adds a new server
    get_next_available_server() -> gets next available server
    
    start() -> starts io_context

    accept() -> calls handle_accept()
    handle_accept() -> creates a new client with next available client_id and calls its read()
    on_client_read_done() -> calls server write()
    on_client_write_done() -> closes the connection with client
    on_server_read_done() -> calls client write(), closes connection with server
    on_server_write_done() -> calls server read()
    
    stop() -> cancels all requests (clients, userclients), closes all sockets

    ~LoadBalancer() -> calls stop()


HealthChecker:
    io_context
    steady_timer
    std::vector<server_details>
    thread -> for closing server connections
    delete_client_list
    mutex -> for delete_client_list

    HealthChecker(period) -> initializes timer, starts thread
    add_server(ip_address, port) -> adds a new server to vector
    std::optional<ServerDetails> get_next_available_server()
    start() -> calls async_wait and runs io_context
    cancel() -> cancels timer
    perform_health_check() -> connects to servers and sends a GET request
    handle_timer_complete() -> calls perform_health_check(), sets new expiry and calls async_wait
    on_server_write_done() -> calls server read
    on_server_read_done() -> sets availability (if no error), closes server connection in separate thread
