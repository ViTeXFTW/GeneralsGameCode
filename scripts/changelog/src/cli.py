from __future__ import annotations

import argparse
import logging
from pathlib import Path
from typing import List

from config import load_config
from github_client import GitHubClient
from github_fetcher import fetch_prs
from yaml_writer import write_pr_yamls

log = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO, format="[%(levelname)s] %(message)s")


def parse_args(argv: List[str] | None = None) -> argparse.Namespace:  # noqa: D401
    """Return CLI arguments."""
    ap = argparse.ArgumentParser(description="Generate YAML of merged GitHub PRs")
    ap.add_argument("--config", required=True, help="Path to JSON config file")
    return ap.parse_args(argv)


def main(argv: List[str] | None = None) -> None:  # noqa: D401
    """Program entrypoint registered in *pyproject.toml*."""
    args = parse_args(argv)
    cfg = load_config(args.config)
    log.info("Loaded config for repo %s", cfg.repository)

    client = GitHubClient(token=cfg.token)
    prs = fetch_prs(cfg, client)
    log.info("Fetched %d PRs", len(prs))

    paths = write_pr_yamls(prs, cfg.output_dir)
    log.info("Wrote %d YAML files to %s", len(paths), cfg.output_dir)


if __name__ == "__main__":  # pragma: no cover
    main()