import pytest
import os

from src.helpers.markdown import lint_pr
from src.helpers.lintTypes import InputType, InputTypeBody, LintError

def test_lint_pr_no_errors():
    sections = [
        InputType("Summary", InputTypeBody.TEXT),
        InputType("Checklist", InputTypeBody.CHECKLIST)
    ]
    
    pr_description = """
    ## Summary
    This PR adds a new feature.

    ## Checklist
    - [x] Tests added
    - [x] Documentation updated
    """
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0, f"Expected no errors, but got:\n{LintError.stringify(errors)}"


def test_lint_pr_missing_summary():
    sections = [
        InputType("Summary", InputTypeBody.TEXT),
        InputType("Checklist", InputTypeBody.CHECKLIST)
    ]
    
    pr_description = """
    ## Checklist
    - [x] Tests added
    - [x] Documentation updated
    """
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 1


def test_lint_pr_missing_checklist():
    sections = [
        InputType("Summary", InputTypeBody.TEXT),
        InputType("Checklist", InputTypeBody.CHECKLIST)
    ]
    
    pr_description = """
    ## Summary
    This PR adds a new feature.
    """
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 1


def test_lint_pr_multiple_errors():
    sections = [
        InputType("Summary", InputTypeBody.TEXT),
        InputType("Checklist", InputTypeBody.CHECKLIST)
    ]
    
    pr_description = """
    ## Checklist
    x Tests added
    """
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 2
    def test_lint_pr_valid_text_section():
        sections = [
            InputType("Summary", InputTypeBody.TEXT)
        ]
        
        pr_description = """
        ## Summary
        This PR fixes a bug.
        """
        errors = lint_pr(pr_description, sections)
        assert len(errors) == 0, f"Expected no errors, but got:\n{LintError.stringify(errors)}"


def test_lint_pr_missing_text_section():
    sections = [
        InputType("Summary", InputTypeBody.TEXT)
    ]
    
    pr_description = """
    ## Checklist
    - [x] Tests added
    """
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 1


def test_lint_pr_valid_checklist_section():
    sections = [
        InputType("Checklist", InputTypeBody.CHECKLIST)
    ]
    
    pr_description = """
    ## Checklist
    - [x] Tests added
    - [x] Documentation updated
    """
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0, f"Expected no errors, but got:\n{LintError.stringify(errors)}"


def test_lint_pr_invalid_checklist_section():
    sections = [
        InputType("Checklist", InputTypeBody.CHECKLIST)
    ]
    
    pr_description = """
    ## Checklist
    x Tests added
    """
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 1
    
# TEXT tests
def test_text_positive():
    sections = [InputType("Summary", InputTypeBody.TEXT, required=True)]
    pr_description = "## Summary\nThis is a valid summary text."
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0

def test_text_negative_empty_body():
    sections = [InputType("Summary", InputTypeBody.TEXT, required=True)]
    pr_description = "## Summary\n"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 1
    assert "empty" in errors[0].message

# BULLETLIST tests
def test_bulletlist_positive():
    sections = [InputType("List", InputTypeBody.BULLETLIST, required=True)]
    pr_description = "## List\n- Item 1\n- Item 2"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0

def test_bulletlist_negative_no_bullet():
    sections = [InputType("List", InputTypeBody.BULLETLIST, required=True)]
    pr_description = "## List\nItem without bullet"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 1
    assert "must start with a bullet" in errors[0].message

# CHECKLIST tests
def test_checklist_positive():
    sections = [InputType("Checklist", InputTypeBody.CHECKLIST, required=True)]
    pr_description = "## Checklist\n- [x] Completed task\n- [ ] Pending task"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0

def test_checklist_negative_invalid_format():
    sections = [InputType("Checklist", InputTypeBody.CHECKLIST, required=True)]
    pr_description = "## Checklist\nx Completed task"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 1
    assert "must start with a checklist item" in errors[0].message

# GitHub Reference tests
def test_gh_reference_positive():
    sections = [InputType("Issue", InputTypeBody.GH_REFERENCE, required=True)]
    pr_description = "## Issue\n#123"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0

def test_gh_reference_negative_invalid():
    sections = [InputType("Issue", InputTypeBody.GH_REFERENCE, required=True)]
    pr_description = "## Issue\n123"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 1
    assert "must reference a GitHub issue or PR" in errors[0].message

# ANY tests
def test_any_positive():
    sections = [InputType("Notes", InputTypeBody.ANY, required=True)]
    pr_description = "## Notes\nAnything goes here, including irregular text!"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0

def test_any_negative_empty_body():
    sections = [InputType("Notes", InputTypeBody.ANY, required=True)]
    pr_description = "## Notes\n"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 1
    assert "empty" in errors[0].message

# Non-required sections should allow empty bodies
def test_non_required_text_empty_body():
    sections = [InputType("Summary", InputTypeBody.TEXT, required=False)]
    pr_description = "## Summary\n"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0

def test_non_required_bulletlist_empty():
    sections = [InputType("List", InputTypeBody.BULLETLIST, required=False)]
    pr_description = "## List\n"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0

def test_non_required_checklist_empty():
    sections = [InputType("Checklist", InputTypeBody.CHECKLIST, required=False)]
    pr_description = "## Checklist\n"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0

def test_non_required_gh_reference_empty():
    sections = [InputType("Issue", InputTypeBody.GH_REFERENCE, required=False)]
    pr_description = "## Issue\n"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0

def test_non_required_any_empty():
    sections = [InputType("Notes", InputTypeBody.ANY, required=False)]
    pr_description = "## Notes\n"
    errors = lint_pr(pr_description, sections)
    assert len(errors) == 0