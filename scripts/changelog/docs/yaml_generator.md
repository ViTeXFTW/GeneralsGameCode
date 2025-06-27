
# YAML Generator

## Table of Contents


### Usage
The YAML generator is a python script located in the `scripts/changelog` directory. By running the `cli.py` it will generate a YAML for each closed merged PR in the chosen repository. The script takes a config file as an argument, which specifies the repository, token, and other relevant settings.

### Config
The config file is a json file that contains the following fields:

| Field | Description | Default |
|-------|-------------|---------|
| `repository` | The GitHub repository to generate the changelog for, in the format `owner/repo` | Required |
| `token` | The GitHub token to use for authentication, leave empty for public repositories | `""` |
| `output_dir` | The directory to output the generated YAML files, from project root | Required |
| `from_date` | The date to start generating the changelog from, in the format `YYYY-MM-DD` | Required |
| `to_date` | The date to stop generating the changelog, in the format `YYYY-MM-DD` | Required |
| `fields` | A list of fields to include in the API request based on [GitHub API documentation](https://docs.github.com/en/rest/pulls/pulls#list-pull-requests) | Required |
| `label_filters` | A list of labels to filter the pull requests by. Only pull requests with these labels will be included | `[]` |
| `state` | The state of the pull requests to include: `open`, `closed`, or `all` | `closed` |
| `per_page` | The number of pull requests to include per page in the API request | `100` |
| `max_title_length` | The maximum length of the pull request title. If longer, it will be truncated | `100` |