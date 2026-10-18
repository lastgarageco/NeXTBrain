"""Host C89/socket checks, not DOS or Watt-32 validation. Requires Python 3.11+."""
import socket
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path


DOS = Path(__file__).resolve().parents[1]
ROOT = DOS.parents[1]


class ClientTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.build = tempfile.TemporaryDirectory()
        cls.executable = Path(cls.build.name) / "nbclient"
        subprocess.run([
            "cc", "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror",
            "-I", str(DOS / "tests"), str(DOS / "nbclient.c"),
            "-o", str(cls.executable),
        ], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.build.cleanup()

    def run_client(self, port, commands, config=None):
        with tempfile.TemporaryDirectory() as directory:
            Path(directory, "NBCLIENT.CFG").write_text(
                config if config is not None else f"HOST=127.0.0.1\nPORT={port}\n"
            )
            return subprocess.run([str(self.executable)], input=commands,
                                  text=True, capture_output=True, cwd=directory, timeout=20)

    def exchange(self, commands, exchanges):
        errors = []
        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            listener.listen()
            listener.settimeout(10)

            def serve():
                try:
                    with listener.accept()[0] as connection:
                        connection.settimeout(10)
                        stream = connection.makefile("rb")
                        for request, chunks in exchanges:
                            self.assertEqual(stream.readline(), request)
                            for chunk in chunks:
                                connection.sendall(chunk)
                                time.sleep(0.05)
                        self.assertEqual(stream.read(), b"")
                except BaseException as error:
                    errors.append(error)

            worker = threading.Thread(target=serve, daemon=True)
            worker.start()
            result = self.run_client(listener.getsockname()[1], commands)
            worker.join(11)
            self.assertFalse(worker.is_alive())
            if errors:
                raise errors[0]
            return result

    def test_split_multiline_large_reply_and_acknowledgments(self):
        result = self.exchange(
            "help\nstatus\nask\nHello\neot\nask\nAgain\nEOL\nquit\n",
            [
                (b"status\n", [b"NeXTBrain Server\n", b"Port       : 5555\n"]),
                (b"Hello\n", [b"First line\n", b"x" * 6000, b"\nLast line\n"]),
                (b"eot\n", [b"O", b"K", b"\n"]),
                (b"Again\n", [b"Fresh reply\n"]),
                (b"EOL\n", [b"OK\n"]),
            ],
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("x" * 6000, result.stdout)
        self.assertIn("Last line\n", result.stdout)
        self.assertIn("Fresh reply\n", result.stdout)
        self.assertNotIn("OK\n", result.stdout)

    def test_invalid_acknowledgment_fails(self):
        result = self.exchange("ASK\nEOT\n", [(b"EOT\n", [b"NO\n"])])
        self.assertEqual(result.returncode, 1)
        self.assertIn("Unexpected conversation-end", result.stdout)

    def test_invalid_configuration(self):
        for config in ("HOST=0.0.0.0\nPORT=5555\n", "HOST=127.0.0.1\nPORT=65536\n",
                       "HOST=127.0.0.1\nPORT=55oops\n", "PORT=5555\n"):
            with self.subTest(config=config):
                self.assertEqual(self.run_client(0, "", config).returncode, 1)

    def test_current_python_server_history_and_reset(self):
        # Use the real server and brain, with a deterministic local AI stand-in.
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            port = probe.getsockname()[1]
        script = """
import sys
from nextbrain.brain import NeXTBrain
from nextbrain.server import NeXTBrainServer
class Provider:
    name = 'Test'
    model = 'test-model'
    def generate(self, messages):
        return 'HISTORY:' + '|'.join(m['content'] for m in messages if m['role'] == 'user')
NeXTBrainServer(NeXTBrain(Provider()), '127.0.0.1', int(sys.argv[1])).serve_forever()
"""
        import os
        environment = dict(os.environ, PYTHONPATH=str(ROOT / "src"), PYTHONDONTWRITEBYTECODE="1")
        server = subprocess.Popen([sys.executable, "-c", script, str(port)], env=environment,
                                  stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        try:
            for attempt in range(100):
                try:
                    with socket.create_connection(("127.0.0.1", port), timeout=0.1):
                        break
                except OSError:
                    time.sleep(0.02)
            else:
                self.fail("Test server did not start")
            result = self.run_client(port, "STATUS\nASK\nAlpha\nBeta\nEOT\nASK\nGamma\nEOL\nQUIT\n")
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("HISTORY:Alpha|Beta", result.stdout)
            self.assertIn("HISTORY:Gamma\n", result.stdout)
            self.assertNotIn("HISTORY:Alpha|Beta|Gamma", result.stdout)
            self.assertIn("Provider   : Test", result.stdout)
        finally:
            server.terminate()
            server.communicate(timeout=5)


if __name__ == "__main__":
    unittest.main()
