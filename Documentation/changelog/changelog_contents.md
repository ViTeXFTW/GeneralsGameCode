# Contents of a PR Changelog
This files specifies what information should carry over to a changelog file when a PR is merged into the main branch. Depending on the type of change, related issues or PRs, the changelog should be updated accordingly. Currently the changelog files are being created manually, but should be automated in the future, when a standard for commit messages and PR layout is established.

## Changelog Structure
The changelog should be structured in the following way:
- **Date**: The date of the PR merge into the main branch.
- **Title**: The title of the PR.
- **Labels**: A list of GitHub labels that are associated with the PR.
- **Affects**: A list of scopes affected by the PR.
- **Changes**: A list of the changes made in the PR. This should be a concise summary of the changes made, including keyword prefix.
- **Links**: A list of links. To the PR itself, to related issues or PRs, and stored within keys specifying the actions taken.
- **Authors**: The author(s) of the PR.

### Example Changelog Entry
```yaml
Date: 2023-10-01
Title: Add underlying types to enum declarations for GCC build
Labels:
  - Build
  - Major
  - ZeroHour
Affects:
  - ZH
Changes:
  - fix: Compilation using MinGW
Links:
  - action: information
    link: https://github.com/TheSuperHackers/GeneralsGameCode/pull/547
  - action: closes
    link: https://github.com/TheSuperHackers/GeneralsGameCode/issues/486
  - action: depends
    link: https://github.com/TheSuperHackers/GeneralsGameCode/pull/888
Authors:
  - ViTeXFTW
  - Xezon
```

### Date
The date the PR was merged into the main branch. This should be in the format YYYY-MM-DD. The date should be the first line of the changelog entry.

### Title
The title of the PR. Copied directly from the PR. This should be the second line of the changelog entry.

### Labels
A list of the labels attached to the PR.
