"""
NeXTBrain

Core application object.

This module contains the main entry point for the NeXTBrain
application. Additional services will be added over time while
keeping the public interface simple and stable.
"""

from nextbrain.providers.provider import AIProvider


class NeXTBrain:
    """Main NeXTBrain application."""

    def __init__(self, provider: AIProvider | None = None):
        """Initialize a NeXTBrain instance."""
        self.version = "0.0.1"
        self.provider = provider

    def __str__(self):
        return f"NeXTBrain v{self.version}"

    def ask(self, prompt: str) -> str:
        """Ask the configured AI provider a question."""

        if self.provider is None:
            raise ValueError("NeXTBrain requires an AI provider.")

        return self.provider.generate(prompt)
