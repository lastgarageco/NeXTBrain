import httpx

from nextbrain.providers.provider import AIProvider


class OpenAIProvider(AIProvider):
    """Text conversation provider backed by the OpenAI Responses API."""

    def __init__(self, model: str, api_key: str):
        self.name = "OpenAI"
        self.model = model
        self.api_key = api_key
        self.timeout = 120.0

    def generate(self, messages: list[dict[str, str]]) -> str:
        """Return reply text without retaining or modifying conversation history."""

        try:
            response = httpx.post(
                "https://api.openai.com/v1/responses",
                headers={"Authorization": f"Bearer {self.api_key}"},
                json={
                    "model": self.model,
                    "input": messages,
                    "stream": False,
                    "store": False,
                },
                timeout=self.timeout,
            )
            response.raise_for_status()
        except httpx.HTTPError:
            # Do not expose request headers or remote error details.
            raise RuntimeError("OpenAI request failed.") from None

        try:
            data = response.json()
            if data["status"] != "completed":
                raise ValueError
            text = "".join(
                part["text"]
                for item in data["output"]
                if item.get("type") == "message"
                for part in item["content"]
                if part.get("type") == "output_text"
            )
            if not text:
                raise ValueError
        except (ValueError, KeyError, TypeError, AttributeError):
            raise RuntimeError("OpenAI did not return a completed text response.") from None

        return text
