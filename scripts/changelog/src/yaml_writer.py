from __future__ import annotations

from pathlib import Path
from typing import Any, Dict, List
from github_fetcher import AppConfig

import yaml
import re


def _clean_title(text: str, max_len: int = 20) -> str:
    """Return ASCII/URL‑friendly slug of first *max_len* chars."""
    striped = re.sub(r"(\[[A-Z]+\]\s*)*", "", text)
    slug = re.sub(r"[^A-Za-z0-9]+", "_", striped[:max_len]).strip("_")
    return slug or "untitled"


def format_issue_keywords(text: str) -> List[str]:
    """Extract issue numbers from text using keywords like 'fixes', 'closes', 'resolves'."""
    patterns = [
        (r"\b(?:fixes)\s+#(\d+)", lambda num: ("Fixes", f"#{num}")),
        (r"\b(?:closes)\s+#(\d+)", lambda num: ("Closes", f"#{num}")),
        (r"\b(?:resolves)\s+#(\d+)", lambda num: ("Resolves", f"#{num}"))
    ]
    
    output = []
    for pattern, formatter in patterns:
        matches = re.findall(pattern, text, re.IGNORECASE)
        output.extend([formatter(num) for num in matches])
    
    return output

def format_pr_keywords(text: str) -> List[str]:
    """Extract PR numbers from text using keywords like 'relates to', 'merge after', 'follow up'."""
    patterns = [
        (r"\b(?:relates[\s]+to)\s+#(\d+)", lambda num: (None, f"Relates To #{num}")),
        (r"\b(?:merge[\s]+after)\s+#(\d+)", lambda num: (None, f"Merge After #{num}")),
        (r"\b(?:follow[\s]+up[\s]+(?:[a-zA-Z0-9]+)*)\s+#(\d+)", lambda num: (f"Follow Up on", f"#{num}")),
        (r"\b(?:Split[\s]+off[\s]+(?:[a-zA-Z0-9]+)*)\s+#(\d+)", lambda num: (f"Split Off from", f"#{num}"))
    ]
    
    output = []
    for pattern, formatter in patterns:
        matches = re.findall(pattern, text, re.IGNORECASE)
        output.extend([formatter(num) for num in matches])
    
    return output


def write_pr_yamls(prs: List[Dict[str, Any]], cfg: AppConfig) -> List[Path]:
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
        clean_title = _clean_title(str(title), cfg.max_title_length)
        
        yaml_content = {
            "PR Number": num,
            "Title": title,
            "Merged At": pr.get("merged_at"),
            "Author": pr.get("user", {}).get("login"),
            "URL": f"https://github.com/{cfg.repository}/pull/{num}",
            "Labels": [label["name"] for label in pr.get("labels", [])],
        }
        
        issues = format_issue_keywords(pr.get("body") or "")
        pull_requests = format_pr_keywords(pr.get("body") or "")
        
        if issues:
            yaml_content["Related Issues"] = [f"{keyword}: {issue}" for keyword, issue in issues]
            
        if pull_requests:
            yaml_content["Related Pull Requests"] = [f"{keyword}: {pr}" if keyword else pr for keyword, pr in pull_requests]
        
        fname = f"{num}_{clean_title}.yaml"
        p = outdir / fname
        p.write_text(yaml.safe_dump(yaml_content, sort_keys=False, allow_unicode=True), encoding="utf-8")
        paths.append(p)
    return paths