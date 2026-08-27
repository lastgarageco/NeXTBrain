import socket

from nextbrain.brain import NeXTBrain


class NeXTBrainServer:
    """Simple TCP server for NeXTBrain."""

    def __init__(
        self,
        brain: NeXTBrain,
        host: str = "0.0.0.0",
        port: int = 5555,
    ):
        self.brain = brain
        self.host = host
        self.port = port

    def serve_forever(self):
        """Start the server and handle client connections."""

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            server.bind((self.host, self.port))
            server.listen()

            print(f"NeXTBrain listening on {self.host}:{self.port}")

            while True:
                connection, address = server.accept()

                with connection:
                    print(f"Connection from {address}")

                    while True:
                        data = b""

                        while not data.endswith(b"\n"):
                            chunk = connection.recv(4096)

                            if not chunk:
                                break

                            data += chunk

                        if not data:
                            break

                        prompt = data.decode("utf-8").strip()

                        response = self.brain.ask(prompt)

                        connection.sendall(
                            (response + "\n").encode("utf-8")
                        )
