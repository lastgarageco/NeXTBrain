from abc import ABC, abstractmethod


class AIProvider(ABC):
    """Base contract for all NeXTBrain AI providers."""

    @abstractmethod
    def generate(self, prompt: str) -> str:
        """Generate a text response for the supplied prompt."""
        raise NotImplementedError
