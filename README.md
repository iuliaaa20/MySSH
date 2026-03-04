# MySSH

A custom, concurrent Remote Shell client-server application built in C for Linux. This project was developed to deeply explore Operating System internals, socket programming, and secure communication protocols. It allows multiple clients to securely connect, authenticate, and execute complex shell commands on the server while being restricted to a sandboxed environment.

## Key Features
1. Concurrent Architecture: Uses a multi-process TCP server leveraging the fork() system call to handle multiple clients simultaneously without blocking.
2. User Sandboxing: Features an internal isolation mechanism that locks authenticated users into their designated workspace directories, preventing unauthorized filesystem traversal.
3. End-to-End Security: Implements a full TLS handshake using X.509 certificates to encrypt all payload traffic.
4. Stateful Execution: Intelligently manages process state, allowing commands like cd to persist across the session by interacting directly with OS-level paths (chdir).
5. Complex Command Handling: Supports executing commands with pipes and redirections by properly interfacing with OS tools via popen.
6. Developer Experience: Client CLI is enhanced with GNU Readline for a smooth, bash-like user experience with history and line editing.
7. Automation: The build process, including source compilation and the generation of cryptographic keys/certificates, is fully automated via a Makefile.

## Technologies & Libraries
1. Language: C (POSIX standard)
2. OS: Linux
3. Networking: Berkeley Sockets API (TCP/IPv4)
4. Security: GnuTLS (libgnutls)
5. CLI: GNU Readline (libreadline)


## Getting Started

### Prerequisites
To build and run this project, you need a Linux environment with the following packages installed:
sudo apt-get update

sudo apt-get install build-essential make libgnutls28-dev libreadline-dev


### Build & Setup

1. Clone the repository: Open your terminal and run git clone https://github.com/iuliaaa20/MySSH.git, then navigate into the directory using cd MySSH.

2. Setup Users: Copy the example user configuration file and add your credentials.

3. Compile and Generate Keys: Use the Makefile to automatically build the client and server binaries, as well as generate the required self-signed TLS certificates. 

### Usage

1. Start the server: Run ./server in your terminal. It listens on port 8000 by default.

2. Start the client: In a separate terminal or on a different machine, start the client by passing the server's IP address as a command-line argument.

  a. For local testing, run: ./client 127.0.0.1

  b. For connecting to a remote server over the network, replace the IP with your server's address. For example: ./client 192.168.1.50
