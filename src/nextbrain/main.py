from nextbrain.brain import NeXTBrain
from nextbrain.config import load_configuration
from nextbrain.providers.ollama import OllamaProvider
from nextbrain.server import NeXTBrainServer


def main():
    config = load_configuration()

    provider_name = config["PROVIDER"]

    if provider_name.lower() != "ollama":
        raise ValueError(
            f"Unsupported provider: {provider_name}"
        )

    provider = OllamaProvider(
        model=config["MODEL"]
    )

    brain = NeXTBrain(
        provider=provider
    )

    server = NeXTBrainServer(
        brain=brain,
        host=config["HOST"],
        port=config["PORT"],
    )

    server.serve_forever()


if __name__ == "__main__":
    main()
