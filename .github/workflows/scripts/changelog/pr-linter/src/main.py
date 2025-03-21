# Argument parser
from helpers.argparser import ArgumentParser
parser = ArgumentParser()

# Github
import helpers.github as github

# Helpers
from helpers.markdown import load_file, lint_pr
from helpers.constants import ALLOWED_SECTIONS
from helpers.logger import logger
from helpers.lintTypes import LintError

if parser.get_args().github:
    github.authenticate_github()

# Load PR text
if parser.get_args().file:
    pr_text = load_file(parser.get_args().file)
elif parser.get_args().pr and parser.get_args().github:
    pr = github.get_pr(parser.get_args().pr)
    pr_text = pr.body
else:
    logger.error("No PR text provided")
    raise SystemExit(1)

# Lint PR
errors = lint_pr(pr_text, ALLOWED_SECTIONS)

if errors:
    logger.error(f"Found {len(errors)}: {LintError.stringify(errors)}")
    github.write_comment_with_errors(pr, errors)
    logger.success("Linting errors found and commented on PR")
    raise SystemExit(1)
else:
    # No errors found; remove any existing lint comment.
    github.delete_comment_with_errors(pr)
    logger.success("No linting errors found; removed stale comment if any")
    raise SystemExit(0)