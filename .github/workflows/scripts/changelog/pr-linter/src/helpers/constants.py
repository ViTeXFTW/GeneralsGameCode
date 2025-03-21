from enum import Enum
from .lintTypes import InputType, InputTypeBody

CI_AUTHOR = {
    "name": "github-actions[bot]",
    "email": "github-actions[bot]@users.noreply.github.com",
}

REQUIRED_HEADERS: list[InputType] = [
    InputType("Description:", InputTypeBody.TEXT),
    InputType("Affects:", InputTypeBody.CHECKLIST),
    InputType("Change list:", InputTypeBody.BULLETLIST),
    InputType("Authors:", InputTypeBody.TEXT),
]
OPTIONAL_HEADERS: list[InputType] = [
    InputType("Fixes Issue(s):", InputTypeBody.GH_REFERENCE, required=False),
    InputType("PRs Update:", InputTypeBody.GH_REFERENCE, required=False),
    InputType("Additional Notes:", InputTypeBody.ANY, required=False),
]
ALLOWED_SECTIONS: list[InputType] = REQUIRED_HEADERS + OPTIONAL_HEADERS

class PR_TAGS(Enum):
    BUGFIX = "bugfix"
    FEATURE = "feature"
    BUILD = "build"
    PERFORMANCE = "performance"
    DOCUMENTATION = "documentation"
    CHORE = "chore"