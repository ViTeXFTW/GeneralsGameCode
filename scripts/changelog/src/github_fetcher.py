from __future__ import annotations

from typing import Any, Dict, List

from config import AppConfig
from github_client import GitHubClient


def fetch_prs(cfg: AppConfig, client: GitHubClient) -> List[Dict[str, Any]]:
    owner, repo = cfg.repository.split("/", 1)
    pr_list: List[Dict[str, Any]] = []
    for pr in client.merged_prs(owner, repo, cfg.state, cfg.per_page):
        if not cfg.is_within_window(pr.get("merged_at")):
            continue
        if cfg.label_filters:
            labels = {lbl["name"] for lbl in pr.get("labels", [])}
            if not labels & set(cfg.label_filters):
                continue
        pr_entry = {field: pr.get(field) for field in cfg.fields}
        pr_list.append(pr_entry)
    return pr_list