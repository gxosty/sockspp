# sockspp

A cross-platform, event-driven SOCKS5 server implementation written in C++.

## Project Goal

The primary goal of sockspp is to provide a robust, cross-platform, and fully asynchronous (event-driven, single-threaded) SOCKS5 server. Many existing C++ SOCKS5 implementations either lack full SOCKS5 command support (e.g., UDP Associate) or rely on traditional multi-threaded architectures, which sockspp aims to avoid. This project intends to implement all SOCKS5 commands except for BIND, focusing on a modern, efficient design.

### Current Features

* CONNECT support

* UDP ASSOCIATE support

* IPv4 and IPv6: Full support for both IPv4 and IPv6 addresses.

* Asynchronous/Event-Driven: Designed from the ground up with an event-driven architecture to avoid the complexities and overhead of multiple threads.

* Cross-Platform: Built with cross-platform compatibility in mind.
  
### Future Plans (To-Do)

The following features are planned for future development:

* Domain Name Resolution: Implementation of domain name resolution for target addresses.

* BIND Command: While not a primary focus, the BIND command might be considered in the future, though it is less common for typical SOCKS5 server use cases.

### Why sockspp?

This project was initiated out of a need for a modern C++ SOCKS5 server that:

* Does not rely on a multi-threaded design for its core operations.

* Aims to support all essential SOCKS5 commands (specifically CONNECT and UDP ASSOCIATE).

* Is designed for cross-platform compatibility.

## Build & Usage

```bash
git clone https://github.com/gxosty/sockspp
cd sockspp
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build . --parallel 4
"./src/server_cli/sockspp-server-cli"
```

## Contributing

Contributions are welcome! If you find bugs, have feature suggestions, or want to contribute code, please feel free to open an issue or pull request on the GitHub repository.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
