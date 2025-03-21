import pytest
import os

from src.helpers.markdown import remove_comments

def test_remove_single_line_comment():
    markdown_content = "<!--- single line comment -->\nThis is a test."
    expected_result = "This is a test."
    assert remove_comments(markdown_content) == expected_result


def test_remove_multiline_comment():
    markdown_content = "<!--- multiline\ncomment -->\nThis is a test."
    expected_result = "This is a test."
    assert remove_comments(markdown_content) == expected_result


def test_remove_bracketed_comment():
    markdown_content = "[//]: # \"asd\"\nThis is a test."
    expected_result = "This is a test."
    assert remove_comments(markdown_content) == expected_result


def test_remove_parenthesized_comment():
    markdown_content = "[//]: # (asd)\nThis is a test."
    expected_result = "This is a test."
    assert remove_comments(markdown_content) == expected_result


def test_remove_inline_comment():
    markdown_content = "[//]: aasd\nThis is a test."
    expected_result = "This is a test."
    assert remove_comments(markdown_content) == expected_result


def test_remove_double_slash_comment():
    markdown_content = "[//]: // \"asd\"\nThis is a test."
    expected_result = "This is a test."
    assert remove_comments(markdown_content) == expected_result


def test_remove_mixed_comment():
    markdown_content = "[//]: asdfasf \"asdf\"\nThis is a test."
    expected_result = "This is a test."
    assert remove_comments(markdown_content) == expected_result
