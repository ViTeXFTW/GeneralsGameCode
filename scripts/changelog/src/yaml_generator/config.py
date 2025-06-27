from __future__ import annotations

from datetime import datetime
from pathlib import Path
from typing import List, Optional, Union

import json
from pydantic import BaseModel, Field, field_validator

RFC3339 = "%Y-%m-%dT%H:%M:%SZ"

class YamlGeneratorConfig(BaseModel):
    """
    Strongly typed representation of JSON config
    """
    
    repository: str
    output_dir: str
    token: Optional[str] = Field(default="", description="GitHub API token, leave empty for unauthenticated requests")
    time_format: str = Field(default=RFC3339, description="Format for timestamps, default is RFC3339")
    from_date: Optional[datetime] = None
    to_date: Optional[datetime] = None
    fields: List[str] = Field(default_factory=lambda: [
        "number",
        "title",
        "user",
        "merged_at",
        "labels"
    ])
    label_filters: Optional[List[str]] = None
    state: str = "closed"
    per_page: int = Field(100, ge=1, le=100)
    max_title_length: int = Field(20, ge=1, le=200)
    
    @field_validator("repository")
    def _repo_must_have_slash(cls, v: str) -> str:
        if "/" not in v:
            raise ValueError("Repository must be in the format 'owner/repo'")
        return v
    
    @field_validator("state")
    def _state_enum(cls, v: str) -> str:
        if v not in ["open", "closed", "all"]:
            raise ValueError("State must be one of 'open', 'closed', or 'all'")
        return v
    
    def is_within_window(self, ts:Optional[str]) -> bool:
        
        if ts is None:
            return False
        dt = datetime.strptime(ts, RFC3339)
        if self.from_date and dt < self.from_date:
            return False
        if self.to_date and dt > self.to_date:
            return False
        return True
    
def load_config(path: Union[str, Path]) -> YamlGeneratorConfig:
    data = json.loads(Path(path).read_text())
    return YamlGeneratorConfig(**data)