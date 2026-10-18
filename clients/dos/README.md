# NeXTBrain DOS client

Small standalone C89 terminal client for **386-or-newer DOS**, targeting
Open Watcom C and Watt-32's 32-bit DOS4GW library. It does not share source
with the NeXTSTEP client. No changes to the NeXTBrain server are required.

## Build

Install Open Watcom with DOS targets and initialize its environment. Obtain
[Watt-32](https://github.com/gvanem/Watt-32) and follow its
[installation instructions](https://github.com/gvanem/Watt-32/blob/master/INSTALL)
to build the Watcom flat-model DOS library (`lib\wattcpwf.lib`) with the BSD
socket API enabled. The library and compiler must use matching memory model
and calling convention (flat model, register arguments). This is not the
16-bit small-model or Windows library.

With the headers and built library in `C:\WATT`, run from this directory:

```bat
SET WATT_ROOT=C:\WATT
BUILD.BAT
```

The batch file invokes:

```bat
wcl386 -bt=dos -l=dos4g -mf -3r -i=%WATT_ROOT%\inc nbclient.c %WATT_ROOT%\lib\wattcpwf.lib -fe=nbclient.exe
```

Use paths without spaces. On DOS, make `DOS4GW.EXE` from your compiler's
runtime available on PATH or beside `NBCLIENT.EXE`. Watt-32 is linked into
the executable; its source and binaries are not bundled here.

## Run

1. Load the DOS packet driver for your network card (or emulated network
   card). A generic DOSBox installation without packet-driver networking
   is insufficient.
2. Configure the DOS machine's networking in `WATTCP.CFG` using Watt-32's
   instructions. Set `WATTCP.CFG` to the **directory** containing that file,
   for example `SET WATTCP.CFG=C:\NET`. This file configures the client's own
   IP, netmask and gateway; it is separate from the NeXTBrain configuration.
3. Copy `CLIENT.CFG` to `NBCLIENT.CFG` and replace `HOST` with your server's
   actual LAN IPv4 address. The supplied `192.0.2.1` is a documentation-only
   placeholder. Use `PORT=5555` or the port configured on the server.
   `0.0.0.0` is a server bind address, not a destination.
4. Start the NeXTBrain server, then run `NBCLIENT` from the directory
   containing `NBCLIENT.CFG`.

Use uppercase `HOST=` and `PORT=` keys with no surrounding spaces. Blank
lines and lines beginning with `#` are allowed. Local `NBCLIENT.CFG` and
compiled artifacts are ignored by Git. Names needed on DOS use 8.3 filenames.

Commands are case-insensitive: `HELP`, `ASK`, `STATUS`, `QUIT`. In ASK mode,
enter a question and wait for the next prompt. `EOT` or `EOL` sends the
conversation-end command and consumes `OK\n` silently; the next ASK starts
fresh on the same connection. Ctrl-Z or Ctrl-C disconnects. Backspace edits
the input; extended keys are ignored. Input is limited to 510 ASCII
characters (a beep indicates the limit). The network stack is serviced
while waiting for keyboard input.

## Protocol and limits

Requests are ASCII lines ending in LF. Replies are received and printed in
chunks, including replies larger than the receive buffer. The existing
server sends UTF-8, but this minimal DOS client displays ASCII and replaces
non-ASCII bytes with `?`; it does not implement Unicode or ANSI escapes.

The current server has **no unambiguous response terminator**: embedded
newlines are allowed and the connection stays open. The client waits up to
180 seconds for data, and treats a one-second idle gap after a newline as
the end of an ordinary reply. This is a LAN-oriented heuristic, not TCP
framing: a longer pause within a multiline reply can produce an early
prompt and misalign later replies. Reconnect if this happens. EOT/EOL uses
the exact `OK\n` acknowledgment instead. No protocol extensions or server
changes are made here.

## Validation

On a POSIX host with `cc` and Python 3.11+, run from the repository root:

```sh
python3 -m unittest discover -s clients/dos/tests -v
```

The tests compile the client as C89 with warnings treated as errors, using
small test-only keyboard/Watt-32 shims and the host's real sockets. They
exercise split replies, large multiline replies, split acknowledgments,
configuration errors, and the actual Python server with a deterministic
test provider, including follow-up history and reset.

Open Watcom, Watt-32 linking, DOS4GW, and packet-driver execution have **not
been validated in the development environment** (no DOS compiler or emulator
installed). Host checks do not establish DOS binary compatibility. Verify
the build and networking on a DOS machine before relying on this client.
