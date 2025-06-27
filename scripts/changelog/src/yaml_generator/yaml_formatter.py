import re
from typing import List, Dict, Any
import yaml

from config import YamlGeneratorConfig

def clean_title(text: str, max_len: int = 20, delimiter: str = "_") -> str:
    """Return ASCII/URL‑friendly slug of first *max_len* chars."""
    striped = re.sub(r"(\[[A-Z]+\]\s*)*", "", text)
    slug = re.sub(r"[^A-Za-z0-9]+", delimiter, striped[:max_len]).strip(delimiter)
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

def format_yaml(pr: Dict[str, Any], cfg: YamlGeneratorConfig) -> str:
    """Format text to YAML-friendly format."""

    yaml_content = {
        "PR Number": pr.get("number"),
        "Title": clean_title(pr.get("title", ""), cfg.max_title_length, " "),
        "Merged At": pr.get("merged_at"),
        "Author": pr.get("user", {}).get("login"),
        "URL": f"https://github.com/{cfg.repository}/pull/{pr.get('number')}",
        "Labels": [label["name"].lower() for label in pr.get("labels", [])],
    }
        
    issues = format_issue_keywords(pr.get("body") or "")
    pull_requests = format_pr_keywords(pr.get("body") or "")
    
    if issues:
        yaml_content["Related Issues"] = [f"{keyword}: {issue}" for keyword, issue in issues]
        
    if pull_requests:
        yaml_content["Related Pull Requests"] = [f"{keyword}: {pr}" if keyword else pr for keyword, pr in pull_requests]
        
    return yaml.safe_dump(yaml_content, sort_keys=False, allow_unicode=True)