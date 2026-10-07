from abc import ABC, abstractmethod


class AIProvider(ABC):
    """Base contract for all NeXTBrain AI providers."""

    @abstractmethod
    def generate(self, messages: list[dict[str, str]]) -> str:
        """Return a reply to ordered role/content messages without modifying them."""
        raise NotImplementedError
