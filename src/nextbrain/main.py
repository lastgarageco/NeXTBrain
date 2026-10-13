from nextbrain.brain import NeXTBrain
from nextbrain.config import load_configuration
from nextbrain.providers.ollama import OllamaProvider
from nextbrain.providers.openai import OpenAIProvider
from nextbrain.server import NeXTBrainServer


def main():
    config = load_configuration()

    provider_name = config["PROVIDER"]

    if provider_name.lower() == "ollama":
        provider = OllamaProvider(model=config["MODEL"])
    elif provider_name.lower() == "openai":
        provider = OpenAIProvider(
            model=config["MODEL"],
            api_key=config["OPENAI_API_KEY"],
        )
    else:
        raise ValueError(
            f"Unsupported provider: {provider_name}"
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
