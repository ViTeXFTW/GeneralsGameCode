import pytest
import os

from src.helpers.markdown import get_body

def test_get_body_with_valid_section():
    markdown_content = """
    ## Section 1
    This is the body of section 1.

    ## Section 2
    This is the body of section 2.
    """
    section = "Section 1"
    expected_body = "This is the body of section 1."

    assert get_body(markdown_content, section) == expected_body

def test_get_body_with_nonexistent_section():
    markdown_content = """
    # Title

    ## Section 1
    This is the body of section 1.
    """
    section = "Section 2"
    assert get_body(markdown_content, section) is None

def test_get_body_with_empty_body():
    markdown_content = """
    # Title

    ## Section 1
    """
    section = "Section 1"
    expected_body = ""
    assert get_body(markdown_content, section) == expected_body

def test_get_body_with_similar_section_names():
    markdown_content = """
    # Title

    ## Section 1
    This is the body of section 1.

    ## Section 1.1
    This is the body of section 1.1.
    """
    section = "Section 1"
    expected_body = "This is the body of section 1."
    assert get_body(markdown_content, section) == expected_body

def test_get_body_with_multiline_body():
    markdown_content = """
    # Title

    ## Section 1
    This is the body of section 1.
    It spans multiple lines.
    """
    section = "Section 1"
    expected_body = "This is the body of section 1.\nIt spans multiple lines."
    assert get_body(markdown_content, section) == expected_body

def test_get_body_with_no_sections():
    markdown_content = """
    # Title

    This is just some text without sections.
    """
    section = "Section 1"
    assert get_body(markdown_content, section) is None