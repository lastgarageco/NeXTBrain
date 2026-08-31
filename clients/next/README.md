# NeXTBrain NeXT Client

`nbclient` is the reference NeXT client for NeXTBrain.

It is a small, text-based TCP client designed to allow a NeXT workstation
to communicate with the NeXTBrain server.

## Tested Environment

The client is currently developed and tested on:

- NeXTSTEP 3.0
- Motorola 68000 family
- GCC 1.93
- BSD sockets

The client communicates with NeXTBrain over TCP.

## Build

From the client directory:

```sh
make
