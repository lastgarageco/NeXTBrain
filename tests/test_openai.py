import contextlib
import io
import tempfile
import traceback
import unittest
from pathlib import Path
from unittest.mock import patch

import httpx

from nextbrain.brain import NeXTBrain
from nextbrain.config import load_configuration
from nextbrain.main import main
from nextbrain.providers.ollama import OllamaProvider
from nextbrain.providers.openai import OpenAIProvider
from nextbrain.server import NeXTBrainServer


class OpenAIConfigurationTests(unittest.TestCase):
    def test_key_is_required_only_for_openai(self):
        for provider in ("OpenAI", "oPeNaI", "Ollama"):
            for key_line in ("", "OPENAI_API_KEY=   \n", "OPENAI_API_KEY=test-secret\n"):
                with self.subTest(provider=provider, key_line=key_line):
                    with tempfile.TemporaryDirectory() as directory:
                        path = Path(directory) / "nbserver.conf"
                        path.write_text(
                            f"HOST=0.0.0.0\nPORT=5555\nPROVIDER={provider}\n"
                            f"MODEL=test-model\n{key_line}"
                        )
                        with patch("nextbrain.config.CONFIG_FILE", path):
                            if provider.lower() == "openai" and "test-secret" not in key_line:
                                with self.assertRaisesRegex(ValueError, "OPENAI_API_KEY"):
                                    load_configuration()
                            else:
                                config = load_configuration()
                                self.assertEqual(config["PORT"], 5555)
                                self.assertEqual(config["MODEL"], "test-model")

    def test_main_selects_provider_without_logging_key(self):
        for name, expected in (("oPeNaI", OpenAIProvider), ("OLLAMA", OllamaProvider)):
            with self.subTest(provider=name):
                config = {"HOST": "0.0.0.0", "PORT": 5555, "PROVIDER": name, "MODEL": "test-model"}
                if expected is OpenAIProvider:
                    config["OPENAI_API_KEY"] = "test-secret"
                output = io.StringIO()
                with patch("nextbrain.main.load_configuration", return_value=config), \
                        patch("nextbrain.main.NeXTBrainServer") as server, \
                        contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
                    main()
                brain = server.call_args.kwargs["brain"]
                self.assertIsInstance(brain.provider, expected)
                self.assertEqual(brain.provider.model, "test-model")
                if expected is OpenAIProvider:
                    self.assertEqual(brain.provider.api_key, "test-secret")
                server.return_value.serve_forever.assert_called_once_with()
                status = NeXTBrainServer(brain, "0.0.0.0", 5555).get_status("127.0.0.1")
                self.assertIn("Version    : 0.0.2", status)
                self.assertIn(f"Provider   : {brain.provider.name}", status)
                self.assertNotIn("test-secret", status + output.getvalue())

    def test_unknown_provider_is_rejected(self):
        with patch("nextbrain.main.load_configuration", return_value={"PROVIDER": "unknown"}):
            with self.assertRaisesRegex(ValueError, "Unsupported provider"):
                main()


class OpenAIProviderTests(unittest.TestCase):
    @patch("nextbrain.providers.openai.httpx.post")
    def test_request_preserves_history_and_extracts_text(self, post):
        provider = OpenAIProvider("test-model", "test-secret")
        messages = [
            {"role": "user", "content": "My name is Ada"},
            {"role": "assistant", "content": "Hello, Ada"},
            {"role": "user", "content": "What is my name?"},
        ]
        original = [message.copy() for message in messages]
        post.return_value = httpx.Response(200, json={
            "status": "completed",
            "output": [
                {"type": "reasoning"},
                {"type": "message", "content": [
                    {"type": "output_text", "text": "Your name is "},
                    {"type": "output_text", "text": "Ada."},
                ]},
            ],
        }, request=httpx.Request("POST", "https://api.openai.com/v1/responses"))

        self.assertEqual(provider.generate(messages), "Your name is Ada.")
        self.assertEqual(messages, original)
        post.assert_called_once_with(
            "https://api.openai.com/v1/responses",
            headers={"Authorization": "Bearer test-secret"},
            json={"model": "test-model", "input": messages, "stream": False, "store": False},
            timeout=120.0,
        )

    @patch("nextbrain.providers.openai.httpx.post")
    def test_http_errors_do_not_expose_key_or_change_history(self, post):
        for status in (401, 429, 500):
            with self.subTest(status=status):
                post.return_value = httpx.Response(status, text="test-secret", request=httpx.Request(
                    "POST", "https://api.openai.com/v1/responses",
                    headers={"Authorization": "Bearer test-secret"},
                ))
                history = []
                with self.assertRaisesRegex(RuntimeError, "OpenAI request failed") as error:
                    NeXTBrain(OpenAIProvider("test-model", "test-secret")).ask("Hello", history)
                self.assertEqual(history, [])
                self.assertNotIn("test-secret", "".join(traceback.format_exception(error.exception)))

        post.side_effect = httpx.ReadTimeout("test-secret")
        with self.assertRaises(RuntimeError) as error:
            OpenAIProvider("test-model", "test-secret").generate([])
        self.assertNotIn("test-secret", "".join(traceback.format_exception(error.exception)))

    @patch("nextbrain.providers.openai.httpx.post")
    def test_incomplete_empty_and_malformed_responses_are_rejected(self, post):
        for data in ({"status": "incomplete"}, {"status": "completed", "output": []}, {"error": "test-secret"}):
            with self.subTest(data=data):
                post.return_value = httpx.Response(200, json=data, request=httpx.Request(
                    "POST", "https://api.openai.com/v1/responses",
                ))
                with self.assertRaisesRegex(RuntimeError, "completed text response") as error:
                    OpenAIProvider("test-model", "test-secret").generate([])
                self.assertNotIn("test-secret", str(error.exception))


if __name__ == "__main__":
    unittest.main()
