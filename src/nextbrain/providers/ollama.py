import httpx

from nextbrain.providers.provider import AIProvider


class OllamaProvider(AIProvider):
    """AI provider backed by an Ollama server."""

    def __init__(self, model: str):
        self.name = "Ollama"
        self.base_url = "http://localhost:11434"
        self.model = model
        self.timeout = 120.0

    def generate(self, prompt: str) -> str:
        """Generate text using Ollama."""

        url = f"{self.base_url}/api/generate"

        payload = {
            "model": self.model,
            "prompt": prompt,
            "stream": False,
        }

        response = httpx.post(
            url,
            json=payload,
            timeout=self.timeout,
        )

        response.raise_for_status()

        data = response.json()

        return data["response"]