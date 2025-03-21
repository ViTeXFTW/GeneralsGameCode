# Libs
import re
import os

# Helpers
from .logger import logger
from .lintTypes import InputType, LintError
from .constants import REQUIRED_HEADERS, OPTIONAL_HEADERS

def load_file(root_file_name: str) -> str:
    """
    Return string of file content.
    """
    
    file_path = os.path.join(os.path.dirname(__file__), "..\\", root_file_name)
    logger.debug(f"Loading file: {file_path[:20] + " ... " + file_path[-20:]}")
    with open(file_path, "r") as f:
        return f.read()

# TODO: Add variable amount of arguments with open/close tags for different comment types
# eksample: remove_comments(text, ("<!--", "-->"), ([//]: #, "\n"))
def remove_comments(string: str) -> str: 
    """
    Return string with comments removed.
    """
    
    # Patterns to match lines that only contain a comment.
    html_comment_pattern = re.compile(r"^\s*<!--.*?-->\s*$", re.DOTALL | re.MULTILINE)
    slash_comment_pattern = re.compile(r"^\s*(?:\[\s*//\s*\]:\s*#.*|//.*)$", re.MULTILINE)
    
    # First remove entire comment lines.
    string = re.sub(html_comment_pattern, "", string)
    string = re.sub(slash_comment_pattern, "", string)
    
    # For inline comments (comments that are a part of a line), remove just the comment part.
    # Adjust these patterns as needed.
    inline_html_pattern = re.compile(r"<!--.*?-->", re.DOTALL)
    inline_slash_pattern = re.compile(r"(?:\[\s*//\s*\]:\s*(\w|/)+).*?$", re.MULTILINE)
    
    string = re.sub(inline_html_pattern, "", string)
    string = re.sub(inline_slash_pattern, "", string)
    
    # Remove any extra empty lines created by removals.
    lines = string.splitlines()
    non_empty_lines = [line for line in lines if line.strip() != ""]
    cleaned_string = "\n".join(non_empty_lines)
            
    if not any(token in string for token in ["<!--", "-->", "[//]:", "//", "(comment:"]):
        logger.success("Removed comments")
    else:
        logger.warning("Failed to remove some comments")
    return cleaned_string

def get_body(string: str, section: str) -> str:
    """
    Return body of section in markdown string.
    """
    lines = string.splitlines()
    body = ""
    
    for i in range(len(lines)):
        line = lines[i].lstrip()
        if not line:
            continue
        
        if line.startswith("##") and line[2:].strip() == section:
            logger.debug(f"Header: {section}")
            for j in range(i+1, len(lines)):
                line = lines[j].lstrip()
                if line.startswith("##"):
                    break
                body += line + "\n"
            logger.debug(f"Body: {body.strip()}")
            
            # Remove leading/trailing newlines
            return body.strip()
        
            
def lint_pr(cleaned_text: str, allowed_sections: list[InputType]) -> list[LintError]:
    """
    Return list of LintErrors based on allowed headers and body text.
    """
    
    
    lint_errors: list[LintError] = []
    _allowed_sections = allowed_sections
    
    # Remove comments from PR text
    cleaned_text = remove_comments(cleaned_text)
    
    # Parse each line in the PR text
    lines = cleaned_text.splitlines()
    for i in range(len(lines)):
        line = lines[i].lstrip()
        if not line:
            continue
        
        # Check for headers
        if line.startswith("##"):
            # Trim leading ##
            header_str = line[2:].strip()

            # Check if section header is required
            for section in _allowed_sections:
                if header_str == section:
                    logger.debug(f"Checking {header_str} section")
                    
                    # Get body of section
                    body = get_body(cleaned_text, header_str)
                    
                    # Validate the body for correct format
                    error = section.validate(body, i)
                    
                    if error:
                        # Append error if found
                        lint_errors.append(error)
                        
                    # Remove section from allowed sections
                    _allowed_sections.remove(section)
    
    # Check for missing headers
    for section in _allowed_sections:
        if section.required:
            lint_errors.append(LintError(-1, f"Missing '{section.header}' section"))
    
    if lint_errors:
        logger.error("Found lint errors")
    else:
        logger.success("No lint errors found")
        
    return lint_errors
