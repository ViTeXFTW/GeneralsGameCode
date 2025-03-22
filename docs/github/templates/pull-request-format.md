# Pull Request Format
To autmatically generate changelog entries, pull requests must follow a specific format. This format is loaded automatically and the markdown file can be found in [here](.github/pull_request_template.md).

## Table of Contents
- [Format](#format)
  - [Description](#description)
  - [Affects](#affects)
  - [Change list](#change-list)
  - [Authors](#authors)
  - [Fixes Issue(s)](#fixes-issues)
  - [PRs Update](#prs-update)
  - [Additional Notes](#additional-notes)

## Format
The format is build up of 4 required sections and 3 optional. The required sections are:
- Description
- Affects
- Change list
- Authors

The optional sections are:
- Fixes Issue(s)
- PRs Update
- Additional Notes

If a section is optional a '?' is added to the end of the header.

### Description
This description should be a detailed explanation of the changes made in this PR. This description will be added to the changelog file and should therefore contain only the relevant information.

### Affects
This section should contain a list of the games that are affected by this PR. The list should be a checkbox list with the following options:
- Generals
- WorldBuilder
- Zero Hour
- Zero Hour WorldBuilder

### Change list
This section should contain a list of the changes made in this PR. The list should be a checkbox list with the following tags:
- bugfix
- feature
- build
- performance
- documentation
- chore

### Authors
List the any additional authors that contributed to this PR.

### Fixes Issue(s)
Link to the issue that this PR fixes.

### PRs Update
Link to another PR that this PR is updating. This is useful when a merged PR is recognized to still have issues and a new PR is created to fix those issues. If the old PR is not a part of a release the new PR will be added to the existing changelog entry.

## Additional Notes
Here you can add any additional information that is not relevant for the changelog. This can be screenshots, videos, tables or any other information that is relevant for the PR but not for the changelog.