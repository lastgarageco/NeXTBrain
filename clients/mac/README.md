# NeXTBrain for Classic Macintosh

A small native **68K, System 7.1 or later** application using the Macintosh
Toolbox, TextEdit, standard controls, and MacTCP. No BSD sockets, Open
Transport API, Carbon, or third-party runtime framework is used.

## System and network setup

- A 68K Macintosh running System 7.1+ with MacTCP installed. MacTCP 2.0.6 is
  the intended networking baseline; it is separate system software on 7.1.
- An Ethernet adapter and its Macintosh driver, or an emulator offering an
  Ethernet interface usable by MacTCP. Configure the Macintosh's own IP,
  subnet and gateway in the MacTCP control panel, then restart as required.
  LocalTalk alone needs an appropriate IP gateway to reach an Ethernet LAN.
- A reachable NeXTBrain server. Its `HOST=0.0.0.0` setting may remain; enter
  its actual LAN IPv4 address in this application's Host field. Port 5555
  is prefilled. No DNS lookup is performed.

The application requests a 512 KB partition (256 KB minimum). It uses only
basic monochrome Toolbox drawing and fits a 512 x 342 screen. Actual memory
requirements and behavior on physical hardware remain unverified.

## Use

Enter Host and Port, then click **Connect**. Type a question and press Return
or click **Send**. Follow-up questions use the same server-side history.
The response area scrolls using its standard scrollbar; text can be selected
and copied with the Edit menu. Tab moves between editable fields. Host and
Port are editable while disconnected; they are entered again each launch.

**Clear** sends `EOT\n` and waits for the exact `OK\n` acknowledgment before
clearing the display. The next question starts a fresh conversation on the
same connection. Offline, Clear just clears the display. **Disconnect** also
discards server-side history. Reconnecting starts with an empty transcript.

Connect, send, receive, abort and release are asynchronous MacTCP operations.
The event loop polls their parameter blocks, allowing redraw, scrolling,
window dragging and application switching while waiting. Send and Clear
are disabled until the current reply finishes. Disconnect can cancel an
operation; File > Quit waits for pending operations and stream release
before exiting. A MacTCP release failure is displayed and retried without
freeing storage that the driver might still own.

Questions are limited to 510 ASCII characters; pasted line breaks become
spaces so one question remains one protocol line. Received LF becomes a
Macintosh carriage return. Non-ASCII UTF-8 bytes display as `?` (no Unicode
conversion in this first client). The transcript discards its oldest text
as needed to stay within classic TextEdit's text and coordinate limits;
this does **not** clear history on the server.

The server reserves bare `STATUS`, `EOT`, and `EOL` lines as commands. Use
the Clear button to end conversations. No authentication or TLS is added
to the existing plain-text TCP connection.

## Protocol limitation

Ordinary server replies have no length header or unique end marker; they
can contain embedded newlines and the connection remains open. This client
uses a one-second receive timeout after a trailing LF as the end of a
reply, and allows up to 180 seconds without data otherwise. A long gap
inside a multiline reply can be mistaken for its end. This LAN-oriented
heuristic is not reliable framing; disconnect/reconnect if replies become
misaligned. Clear's `OK\n` acknowledgment is matched across split receives.
The Python server and other clients are unchanged.

## Build

Sources are C89 and use the non-opaque classic Toolbox structures. Use a
68K compiler and Apple's Universal Interfaces, including `MacTCP.h` and
the matching Rez `Types.r`. The SDK and system software are not bundled.
No MacTCP helper library is needed: networking calls the `.IPP` driver
through Device Manager `PBControl` directly.

### Retro68 cross-build

Build/install [Retro68](https://github.com/autc04/Retro68) with **Apple
Universal Interfaces 3.x** (3.4 is the upstream-tested version). Retro68's
default Multiversal Interfaces do not supply MacTCP. From `clients/mac`:

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
cmake --build build
```

Use the resulting application/MacBinary or disk-image artifact produced by
Retro68. Preserve the application's resource fork when transferring to the
Macintosh; copying just an ELF binary or a data fork will not work. This
target intentionally rejects the PowerPC toolchain.

### Classic development tools

In a 68K CodeWarrior or MPW C environment with compatible Universal
Interfaces, create a normal Macintosh application (not a console tool or
CFM application). Compile `main.c`, `network.c`, and `wire.c` as C; add
`NeXTBrain.r` to the resource build. Define `TARGET_API_MAC_CARBON=0` and
`OPAQUE_TOOLBOX_STRUCTS=0`. Link the compiler's normal 68K Toolbox interface
and C runtime libraries. Keep the SDK's standard 68K structure alignment,
especially for `TCPiopb` and `wdsEntry`. No separate socket library is linked.
The native project setup and Retro68 recipe have not been compiled here.

## Testing and current validation

On a development host with a C compiler, from the repository root:

```sh
sh clients/mac/tests/run.sh
```

This compiles `wire.c` and `network.c` as strict C89 with warnings treated
as errors. A fake Device Manager tests address parsing, request formatting,
text conversion, asynchronous connect/send/receive, split acknowledgments,
timeout handling and cancellation. Test headers are deliberately incomplete
host declarations, **not** SDK replacements and **not** a MacTCP emulator.
Do not add the `tests` include directory to a Macintosh build.

**Not yet tested:** compiling/linking the Toolbox application with a 68K SDK,
Rez compilation, the visual layout, actual MacTCP driver behavior, and
physical Macintosh or emulator execution. The current development machine
has no Retro68/classic compiler, classic SDK, or Macintosh emulator. The
modern macOS `Rez` tool alone cannot validate a classic application.

For emulator acceptance testing, use a 68K Basilisk II setup capable of
running your licensed System 7.1 image (or another compatible later System
7 image), with Ethernet networking exposed to the guest. Supply your own
ROM/system software and install/configure MacTCP. Verify LAN connectivity
before launching. Then check:

1. Connect to NeXTBrain; send two questions where the second needs history.
2. Scroll and copy a long response; move the window and switch applications
   while waiting for a slow response.
3. Clear, then verify the next question has no earlier context.
4. Disconnect or Quit while connecting and while waiting for a reply; check
   that no crash or stale driver buffer remains. Reconnect repeatedly.
5. Try an unreachable address, stopped server, invalid port and missing
   MacTCP; check errors and button states.
6. Check redraw, clipboard, keyboard navigation and scroll limits on a
   small monochrome display, both 24-bit and 32-bit addressing where supported.

MacTCP reference: Apple's
[MacTCP Programmer's Guide](https://bitsavers.org/pdf/apple/mac/developer/Networking/MacTCP_Programmers_Guide_1989.pdf).
