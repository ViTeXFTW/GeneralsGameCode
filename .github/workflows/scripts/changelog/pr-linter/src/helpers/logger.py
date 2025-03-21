from loguru import logger
import sys

from helpers.argparser import ArgumentParser
parser = ArgumentParser()

if parser.get_args().debug:
    logger.remove(0)
    logger.add(
        sys.stderr,
        level="DEBUG",
        format="<green>{time:HH:MM:SS}</green> | "
            "<level>{level}</level> | "
            "{message}",
        colorize=True
    )
else:
    logger.remove(0)
    logger.add(
        sys.stderr,
        level="INFO",
        format="<green>{time:HH:MM:SS}</green> | "
            "<level>{level}</level> | "
            "{message}",
        colorize=True
    )