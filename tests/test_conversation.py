import unittest
from unittest.mock import MagicMock, Mock, call, patch

import httpx

from nextbrain.brain import NeXTBrain
from nextbrain.providers.ollama import OllamaProvider
from nextbrain.server import NeXTBrainServer


class ConversationTests(unittest.TestCase):
    def test_follow_up_includes_previous_question_and_answer(self):
        provider = Mock()
        provider.generate.side_effect = ["Hello, Ada.", "Ada"]
        brain = NeXTBrain(provider)
        history = []

        brain.ask("My name is Ada.", history)
        self.assertEqual(brain.ask("What is my name?", history), "Ada")

        first_turn = [
            {"role": "user", "content": "My name is Ada."},
            {"role": "assistant", "content": "Hello, Ada."},
        ]
        question = {"role": "user", "content": "What is my name?"}
        self.assertEqual(provider.generate.call_args_list, [
            call(first_turn[:1]),
            call(first_turn + [question]),
        ])
        self.assertEqual(history, first_turn + [
            question, {"role": "assistant", "content": "Ada"},
        ])

    def test_failed_generation_does_not_change_history(self):
        provider = Mock()
        provider.generate.side_effect = RuntimeError("Provider unavailable")
        history = [
            {"role": "user", "content": "Hello"},
            {"role": "assistant", "content": "Hi"},
        ]
        original = history.copy()

        with self.assertRaises(RuntimeError):
            NeXTBrain(provider).ask("Follow-up", history)

        self.assertEqual(history, original)

    def test_connections_have_separate_history_and_status_is_excluded(self):
        provider = Mock()
        provider.generate.side_effect = ["First answer", "Second answer", "New answer"]
        server = NeXTBrainServer(NeXTBrain(provider), "0.0.0.0", 5555)
        first = MagicMock()
        first.recv.side_effect = [b"Hello\n", b"STATUS\n", b"Follow-up\n", b""]
        first.getsockname.return_value = ("10.0.0.2", 5555)
        second = MagicMock()
        second.recv.side_effect = [b"New conversation\n", b""]

        with patch("nextbrain.server.socket.socket") as socket_factory:
            listener = socket_factory.return_value.__enter__.return_value
            listener.accept.side_effect = [
                (first, ("10.0.0.3", 10000)),
                (second, ("10.0.0.4", 10001)),
                StopIteration,
            ]
            with patch("builtins.print"), self.assertRaises(StopIteration):
                server.serve_forever()

        self.assertEqual(provider.generate.call_args_list, [
            call([{"role": "user", "content": "Hello"}]),
            call([
                {"role": "user", "content": "Hello"},
                {"role": "assistant", "content": "First answer"},
                {"role": "user", "content": "Follow-up"},
            ]),
            call([{"role": "user", "content": "New conversation"}]),
        ])
        self.assertEqual(first.sendall.call_args_list, [
            call(b"First answer\n"),
            call((server.get_status("10.0.0.2") + "\n").encode("utf-8")),
            call(b"Second answer\n"),
        ])
        second.sendall.assert_called_once_with(b"New answer\n")

    def test_eot_and_eol_start_a_fresh_conversation(self):
        for command in (b"EOT\n", b"eol\n"):
            with self.subTest(command=command):
                provider = Mock()
                provider.generate.side_effect = ["Old answer", "New answer"]
                server = NeXTBrainServer(NeXTBrain(provider), "0.0.0.0", 5555)
                connection = MagicMock()
                connection.recv.side_effect = [
                    b"Old question\n", command, b"New question\n", b"",
                ]
                with patch("nextbrain.server.socket.socket") as socket_factory:
                    listener = socket_factory.return_value.__enter__.return_value
                    listener.accept.side_effect = [
                        (connection, ("10.0.0.3", 10000)), StopIteration,
                    ]
                    with patch("builtins.print"), self.assertRaises(StopIteration):
                        server.serve_forever()

                self.assertEqual(provider.generate.call_args_list, [
                    call([{"role": "user", "content": "Old question"}]),
                    call([{"role": "user", "content": "New question"}]),
                ])
                self.assertEqual(connection.sendall.call_args_list, [
                    call(b"Old answer\n"), call(b"OK\n"), call(b"New answer\n"),
                ])

    @patch("nextbrain.providers.ollama.httpx.post")
    def test_ollama_chat_request_and_response(self, post):
        provider = OllamaProvider("test-model")
        messages = [{"role": "user", "content": "Hello"}]
        post.return_value = httpx.Response(
            200,
            json={"message": {"role": "assistant", "content": "Hi"}},
            request=httpx.Request("POST", "http://localhost:11434/api/chat"),
        )

        self.assertEqual(provider.generate(messages), "Hi")
        post.assert_called_once_with(
            "http://localhost:11434/api/chat",
            json={"model": "test-model", "messages": messages, "stream": False},
            timeout=120.0,
        )
        self.assertEqual(messages, [{"role": "user", "content": "Hello"}])


if __name__ == "__main__":
    unittest.main()
