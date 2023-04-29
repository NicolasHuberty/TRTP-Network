# TRTP Network Sender and Receiver

TRTP Network Sender and Receiver is a pair of programs designed to transmit data over a network using the UDP protocol. This repository provides an implementation of both the sender and the receiver, complete with error handling and logging capabilities.

## Table of Contents

- [Introduction](#introduction)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
  - [Sender](#sender)
  - [Receiver](#receiver)
- [Contributing](#contributing)
- [License](#license)

## Introduction

This project consists of two separate programs, `sender` and `receiver`, which are designed to transmit and receive data packets over a network. The sender reads data from a file or standard input and transmits it to the receiver using UDP. The receiver listens for incoming packets, processes them, and writes the received data to a file or standard output.

## Requirements

- GCC compiler
- GNU Make

## Installation

1. Clone the repository:
```bash
git clone https://github.com/NicolasHuberty/TRTP-Network.git
```

2. Enter the project directory:
```bash
cd TRTP-Network
```
3. Compile the sender and receiver programs:
```bash
make all
```
## Usage

### Sender

To run the sender program, use the following syntax:

```bash
./sender [-f filename] [-s stats_filename] [-c] receiver_ip receiver_port
```

Options:

- `-f filename`: The input file to read data from. If not specified, the sender reads from standard input.
- `-s stats_filename`: The file to write transmission statistics to. If not specified, the statistics are printed to standard output.
- `-c`: Enable forward error correction (currently not implemented).
- `receiver_ip`: The IP address of the receiver.
- `receiver_port`: The port number the receiver is listening on.

### Receiver

To run the receiver program, use the following syntax:
```bash
./receiver [-s stats_filename] listen_ip listen_port
```

Options:

- `-s stats_filename`: The file to write reception statistics to. If not specified, the statistics are printed to standard output.
- `listen_ip`: The IP address to bind the receiver to.
- `listen_port`: The port number to listen on for incoming data.

## Contributing

Contributions are welcome! Please submit your changes as a pull request and follow the project's coding style and conventions.

## License

This project is released under the MIT License. See the [LICENSE](LICENSE) file for more information.

