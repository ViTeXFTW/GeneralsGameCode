from __future__ import annotations

from pathlib import Path
from typing import Any, Dict, List
from config import YamlGeneratorConfig
from yaml_formatter import format_yaml, clean_title

import re

def write_pr_yamls(prs: List[Dict[str, Any]], cfg: YamlGeneratorConfig) -> List[Path]:
    """Write a separate YAML file for each PR.

    Filenames are in form ``{number}_{slug}.yaml``.
    Returns list of written :class:`Path` objects.
    """
    outdir = Path(cfg.output_dir)
    outdir.mkdir(parents=True, exist_ok=True)
    paths: List[Path] = []
    for pr in prs:
        num = pr.get("number")
        title = pr.get("title", "")
        _clean_title = clean_title(str(title), cfg.max_title_length).lower()
        
        fname = f"{num}_{_clean_title.lower()}.yaml"
        p = outdir / fname
        p.write_text(format_yaml(pr, cfg), encoding="utf-8")
        paths.append(p)
    return paths