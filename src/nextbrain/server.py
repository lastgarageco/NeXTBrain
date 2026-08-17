import socket

from nextbrain.brain import NeXTBrain


class NeXTBrainServer:
    """Simple TCP server for NeXTBrain."""

    def __init__(
            self, 
            brain: NeXTBrain, 
            host="0.0.0.0", 
            port=5555):
        self.brain = brain
        self.host = host
        self.port = port

    def serve_forever(self):
        """Start the server and handle client requests."""

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            server.bind((self.host, self.port))
            server.listen()

            print(f"NeXTBrain listening on {self.host}:{self.port}")

            while True:
                connection, address = server.accept()

                with connection:
                    print(f"Connection from {address}")

                    data = connection.recv(4096)

                    if not data:
                        continue

                    prompt = data.decode("utf-8").strip()

                    response = self.brain.ask(prompt)

                    connection.sendall(response.encode("utf-8"))
