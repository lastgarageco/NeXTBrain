from nextbrain.brain import NeXTBrain
from nextbrain.providers.ollama import OllamaProvider
from nextbrain.server import NeXTBrainServer


def main():
    """Start the NeXTBrain server."""

    provider = OllamaProvider()

    brain = NeXTBrain(
        provider=provider
    )

    server = NeXTBrainServer(
        brain=brain
    )

    server.serve_forever()


if __name__ == "__main__":
    main()
