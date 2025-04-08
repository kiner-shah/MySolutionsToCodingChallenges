## Requirements
- Asio 1.30.2
- spdlog 1.15.2 (used compiled version, not header-only version)


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

