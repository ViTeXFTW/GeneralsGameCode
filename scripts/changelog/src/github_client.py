from __future__ import annotations

import logging
from datetime import datetime
from typing import Any, Dict, Generator, List

import requests

log = logging.getLogger(__name__)
ENDPOINT = "https://api.github.com"

class GitHubClient:
    
    def __init__(self, token: str | None = None, session: requests.Session | None = None):
        self.session = session or requests.Session()
        if token:
            self.session.headers.update({"Authorization": f"token {token}"})
        self.session.headers.update({"Accept": "application/vnd.github+json"})
        
    def merged_prs(self, owner: str, repo: str, state: str = "closed", per_page: int = 100) -> Generator[Dict[str, Any], None, None]:
        
        page = 1
        while True:
            url = f"{ENDPOINT}/repos/{owner}/{repo}/pulls"
            params = {
                "state": state,
                "per_page": per_page,
                "page": page
            }
            res = self.session.get(url, params=params, timeout=30)
            data: List[Dict] = res.json()
            
            if not data:
                break
            for pr in data:
                if pr.get("merged_at"):
                    yield pr
                    
            page += 1