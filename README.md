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
´´´bash

git clone https://github.com/NicolasHuberty/TRTP-Network.git
´´

2. Enter the project directory:

cd TRTP-Network

3. Compile the sender and receiver programs:

make all


## Usage

### Sender

To run the sender program, use the following syntax:

´´´
./sender [-f filename] [-s stats_filename] [-c] receiver_ip receiver_port
´´
