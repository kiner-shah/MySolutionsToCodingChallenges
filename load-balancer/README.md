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