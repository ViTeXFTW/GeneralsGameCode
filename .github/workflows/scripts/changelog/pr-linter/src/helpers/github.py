# Libs
import os
from dotenv import load_dotenv
load_dotenv()

# Github
from github import Github, Repository, PullRequest, NamedUser

# Helpers
from .logger import logger
from .lintTypes import LintError

github: Github = None
repository: Repository.Repository = None

def authenticate_github(token: str = os.getenv("GH_TOKEN"), repo: str = os.getenv("GH_REPOSITORY")):
    """
    Return authenticated GitHub API and repository.
    """
    global github, repository
    
    github = Github(token)
    if not github:
        logger.error("Failed to authenticate with GitHub")
        raise SystemExit(1)
    
    repository = github.get_repo(repo)
    if not repo:
        logger.error("Failed to get repository")
        raise SystemExit(1)
    
    logger.success("Authenticated with GitHub")

def get_pr(pr_number: int) -> PullRequest.PullRequest | None:
    """
    Return PR from PR number on GitHub.
    """
    
    if not repository:
        logger.error("GitHub API has not been initialized")
        return None
    
    pr = repository.get_pull(pr_number)
    if not pr:
        logger.error(f"Failed to get PR with number {pr_number}")
        raise SystemExit(1)
    
    return pr

def write_comment_with_errors(pr: PullRequest.PullRequest, errors: list[LintError]):
    """
    Write a comment on a PR with the linting errors.
    If a comment with "## Linting Errors" already exists, edit it.
    """
    
    if not pr:
        logger.error("PR has not been initialized")
        return
    
    new_comment = "## Linting Errors\n"
    for error in errors:
        new_comment += f"- {error}\n"
    
    # Check for an existing comment with the heading
    existing_comment = None
    for comment in pr.get_issue_comments():
        if comment.body.startswith("## Linting Errors"):
            existing_comment = comment
            break
    
    if existing_comment:
        existing_comment.edit(new_comment)
        logger.info("Edited existing comment with linting errors")
    else:
        pr.create_issue_comment(new_comment)
        logger.info("Created new comment with linting errors")
    
    return

def delete_comment_with_errors(pr: PullRequest.PullRequest):
    """
    Delete an existing linting errors comment on the PR if it exists.
    """
    
    for comment in pr.get_issue_comments():
        if comment.body.startswith("## Linting Errors"):
            comment.delete()
            logger.info("Deleted the linting errors comment")
            break