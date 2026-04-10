"""Entrypoint for the copy-trading bot."""
from __future__ import annotations

import logging
import sys

from pmbot.config import ConfigError, load_config
from pmbot.copier import CopyTraderBot
from pmbot.polymarket_client import ClientError, load_client
from pmbot.storage import StateStore


def configure_logging() -> None:
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )


def main() -> int:
    configure_logging()
    try:
        config = load_config()
        client = load_client()
        store = StateStore(config.state_path)
        bot = CopyTraderBot(config, client, store)
        bot.poll_forever()
    except (ConfigError, ClientError) as exc:
        logging.getLogger(__name__).error(str(exc))
        return 1
    except KeyboardInterrupt:
        logging.getLogger(__name__).info("Shutdown requested")
        return 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
