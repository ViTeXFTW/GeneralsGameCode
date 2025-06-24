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
    
    def _check_response(self, res: requests.Response) -> bool:
        """ Check the response status code and log errors if necessary."""
        if res.status_code == 200:
            return True
        elif res.status_code == 404:
            log.error("Resource not found: %s", res.url)
            return False
        elif res.status_code == 403:
            log.error("Forbidden: %s. Check your token permissions.", res.url)
            return False
        else:
            log.error("Error %d: %s", res.status_code, res.text)
            res.raise_for_status()
            return False
    
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
            
            if not self._check_response(res):
                break
            
            data: List[Dict] = res.json()
            
            if not data:
                break
            for pr in data:
                if pr.get("merged_at"):
                    yield pr
                    
            page += 1
            
    def get_pr(self, owner: str, repo: str, pr_number: int) -> Dict[str, Any]:
        url = f"{ENDPOINT}/repos/{owner}/{repo}/pulls/{pr_number}"
        res = self.session.get(url, timeout=30)
        if res.status_code == 404:
            raise ValueError(f"PR #{pr_number} not found in {owner}/{repo}")
        res.raise_for_status()
        return res.json()