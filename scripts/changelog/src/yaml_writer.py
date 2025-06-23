from __future__ import annotations

from pathlib import Path
from typing import Any, Dict, List

import yaml
import re


def _slugify(text: str, max_len: int = 20) -> str:
    """Return ASCII/URL‑friendly slug of first *max_len* chars."""
    slug = re.sub(r"[^A-Za-z0-9]+", "-", text[:max_len]).strip("-")
    return slug or "untitled"


def write_pr_yamls(prs: List[Dict[str, Any]], output_dir: str | Path) -> List[Path]:
    """Write a separate YAML file for each PR.

    Filenames are in form ``{number}_{slug}.yaml``.
    Returns list of written :class:`Path` objects.
    """
    outdir = Path(output_dir)
    outdir.mkdir(parents=True, exist_ok=True)
    paths: List[Path] = []
    for pr in prs:
        num = pr.get("number")
        title = pr.get("title", "")
        slug = _slugify(str(title))
        fname = f"{num}_{slug}.yaml"
        p = outdir / fname
        p.write_text(yaml.safe_dump(pr, sort_keys=False, allow_unicode=True))
        paths.append(p)
    return paths