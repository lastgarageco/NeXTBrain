"""
Configuration loader for the NeXTBrain server.
"""

from pathlib import Path

CONFIG_FILE = Path("nbserver.conf")

REQUIRED_KEYS = (
    "HOST",
    "PORT",
    "PROVIDER",
    "MODEL",
)


def load_configuration():
    """
    Load NeXTBrain server configuration from nbserver.conf.

    Returns:
        dict containing:
            HOST
            PORT
            PROVIDER
            MODEL

    Raises:
        FileNotFoundError:
            If nbserver.conf does not exist.

        ValueError:
            If a required setting is missing or invalid.
    """

    if not CONFIG_FILE.exists():
        raise FileNotFoundError(
            f"Configuration file '{CONFIG_FILE}' not found."
        )

    config = {}

    with CONFIG_FILE.open("r", encoding="utf-8") as file:
        for line in file:
            line = line.strip()

            if not line:
                continue

            if line.startswith("#"):
                continue

            if "=" not in line:
                continue

            key, value = line.split("=", 1)

            key = key.strip().upper()
            value = value.strip()

            config[key] = value

    for key in REQUIRED_KEYS:
        if key not in config or not config[key]:
            raise ValueError(
                f"Missing configuration value: {key}"
            )

    try:
        config["PORT"] = int(config["PORT"])
    except ValueError:
        raise ValueError(
            "PORT must be a valid integer."
        )

    if config["PORT"] < 1 or config["PORT"] > 65535:
        raise ValueError(
            "PORT must be between 1 and 65535."
        )

    return config