Implemented Redis server  as per the [challenge description](https://codingchallenges.fyi/challenges/challenge-redis).

## Dependencies
- [Asio 1.30.2](https://think-async.com/Asio/)
- [spdlog 1.15.2](https://github.com/gabime/spdlog/releases/tag/v1.15.2) (used compiled version, not header-only version)

These dependencies should be present in the project root directory.
- spdlog should be compiled into a library and the install folder should be present in the project root directory
- asio should be downloaded and extracted in the project root directory

## Scribble
RedisServer
- vector<ClientPtr>
- io_context
- port
- executor_work_guard
- acceptor

- RedisServer(port)
- start()
- accept()
- handle_accept()
- on_read_done()
- on_write_done()
- stop()
- ~RedisServer()

Client
- io_context
- executor_work_guard
- socket
- client_id
- read_buffer
- write_buffer
- on_read_complete
- on_write_complete

- Client()
- write(data)
- handle_write()
- read()
- handle_read()
- ~Client()