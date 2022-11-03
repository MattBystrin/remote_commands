# Remote commands
Server also accepts blocking commands, in example "top", and can work both in single-threaded and multi-threaded mode.

Server help:
```
Usage: server [OPTION...]
Server for remote command execution

  -a, --addr=ADDR            Server addres
  -p, --port=PORT            Server port
  -t, --threads=NUM          Threads number
  -?, --help                 Give this help list
      --usage                Give a short usage message

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
```
Example:
```
./server -a 127.0.0.1 -p 60000 -t 4
```
Clients help:
```
Usage: client [OPTION...] COMMAND
Client for remote command execution

  -a, --addr=ADDR            Server addres
  -p, --port=PORT            Server port
  -?, --help                 Give this help list
      --usage                Give a short usage message

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
```
Example
```
./client -a 127.0.0.1 -p 60000 "ls -l"
```
# Notes
Server accept only limited amount of peers which setup during compile time.

Client and a server use a simple protol.
1) Client sends request with format: | command_size (4 bytes) | command |
2) Server receiver request and start executing command and sends output to client


