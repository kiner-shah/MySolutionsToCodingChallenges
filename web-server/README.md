Implemented web server as per the [challenge description](https://codingchallenges.fyi/challenges/challenge-webserver).

## Dependencies
- [Asio 1.30.2](https://think-async.com/Asio/)
- [spdlog 1.15.2](https://github.com/gabime/spdlog/releases/tag/v1.15.2) (used compiled version, not header-only version)

These dependencies should be present in the project root directory.
- spdlog should be compiled into a library and the install folder should be present in the project root directory
- asio should be downloaded and extracted in the project root directory

## Running simple tests
```
sh tests.sh
```

## Notes
- This is just a basic web server as per the challenge description not a full fledged web server that supports all types of requests, and other parts of HTTP. For example,
    - Content-Type is not sent in the responses for now.
    - Only GET requests are supported. POST, PUT, PATCH, etc. requests are not supported for now.
