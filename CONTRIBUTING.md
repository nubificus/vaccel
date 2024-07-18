# Contributing to vAccel

Thank you for your interest in vAccel. We appreciate any effort to improve our
project and welcome contributions from the community. Please take a moment to
review the guidelines below to facilitate a smooth and effective contribution
process.

## How to contribute

1. **Use and Report:** Try vAccel and share your experience. Were the
  instructions clear? Did everything work as expected?
2. **Improve Documentation:** Suggest changes or improvements to the
  documentation to make it clearer.
3. **Request Features:** Propose new features or enhancements.
4. **Report Bugs:** Describe any issues you encounter.
5. **Code Changes:** Improve existing code or add new functionality.

## Code organization

The vAccel project is structured as follows:
- **Root Directory:** Contains build files, LICENSE, README and other non-code
  files.
- **`/src`:** Contains the core implementation of the vAccel library and its'
  headers.
- **`/plugins`:** Contains the implementation of core vAccel plugins.
- **`/examples`:** Provides examples demonstrating library/API usage.
- **`/python`:** Includes a library implementation for vAccel python
  integration.
- **`/docs`:** Houses project documentation.
- **`/scripts`:** Contains scripts used for building and distributing vAccel.
- **`/subprojects`:** Holds Meson-specific files for fetching/compiling build
  dependencies.
- **`/test`:** Includes unit and integration tests to verify library
  functionality.

## Opening an issue

If you encounter any bugs or have suggestions for improvements, please open an
[issue](https://github.com/nubificus/vaccel/issues/new).

### Reporting bugs

When reporting a bug provide as much detail as possible, including steps to
reproduce the issue and any relevant information about your environment.
Specifically, include:
- A clear description of the issue.
- The respective output logs using the maximum debug level
  (`VACCEL_DEBUG_LEVEL=4`).
- Version details of vAccel and plugins (either the commit's hash or the
  version).
- CPU architecture and build type.
- Steps to reproduce the issue.
- Mark the issue with the `bug` label.
- Keep an eye on the issue for possible questions from the maintainers.

Example template for an issue:
```text
## Description
An explanation of the issue 

## System info
- vAccel version:
- vAccel Plugin/Plugin's version:
- Arch:
- Build type:

## Steps to reproduce
A list of steps to reproduce the issue.
```

### Requesting new features

To make a feature request, mark the issue with the `enhancement` label and
provide a detailed description of the proposed feature.

## Submitting a Pull Request

For significant changes, open an issue to discuss and receive feedback before
proceeding with the implementation.

### Building vAccel and testing the changes

You can find information on building vAccel in [README](README.md) and
[meson_build](docs/meson_build.md) docs. After implementing the desired changes,
the project should build and install correctly. Additionally, code
[tests](docs/meson_build.md#running-the-tests) and
[examples](docs/meson_build.md#running-the-examples) should be implemented
(or modified) to include the new functionality. The process of formatting and
running static analysis on the code is described
[here](docs/meson_build.md#formatting-code-and-running-static-analysis).

### Preparing for a Pull Request

Before creating a new Pull Request, ensure that:
- The build process is not broken.
- Your code includes appropriate tests and all tests pass.
- The style used follows the coding style of the project.

To submit changes:
- Use one commit for each new feature or changed functionality
- Ensure that no commit in the Pull Request breaks the project's build process
- Make sure to sign-off your commits
- Provide meaningful commit messages, describing shortly the changes the commit
  introduces
- Provide a meaningful Pull Request title and message

### Pull Request process

1. **Automated checks:** A maintainer checks the Pull Request and triggers the
  automated checks.
2. **Review:** If the checks pass, one or more maintainers review the Pull
  Request.
3. **Address comments:** The author of the Pull Request needs to address all the
  reviewer comments.
4. **Approval:** Upon approval, git trailers are added to the Pull Requests's
  commits.
5. **Merge:** The Pull Request is merged.

## Style guide

### Git commit messages

Write clear and concise commit messages. Follow these guidelines for your
commit messages:
- Limit the header (first line) to 72.
- Limit the body/footer to 80 characters
- Follow the [Conventional Commits](https://www.conventionalcommits.org/)
  specification and, specifically, format the header as `<type>[optional scope]:
  <description>`, where `description` must not end with a fullstop and `type`
  can be one of:
  - *feat*: A new feature
  - *fix*: A bug fix
  - *docs*: Documentation only changes
  - *style*: Changes that do not affect the meaning of the code (white-space,
    formatting, missing semi-colons, etc)
  - *refactor*: A code change that neither fixes a bug nor adds a feature
  - *perf*: A code change that improves performance
  - *test*: Adding missing tests
  - *build*: Changes that affect the build system or external dependencies
    (example scopes: gulp, broccoli, npm)
  - *ci*: Changes to our CI configuration files and scripts (example scopes:
    Travis, Circle, BrowserStack, SauceLabs)
  - *chore*: Other changes that don't modify src or test files
  - *revert*: Reverts a previous commit
- If the PR is associated with an issue, reference issues with
  `Fixes: #issue_number`
- Always sign-off your commit message

The above rules are enforced by the PR checks, which will fail if requirements
are not met.

### C/C++ code style

vAccel code is formatted using `clang-format` with custom rules based on the
Linux Kernel/K&R style. Contributors can automatically apply formatting rules
using the ninja target `clang-format` generated my meson. When Pull Request
checks are run, an automated review with a patch containing any additional
format changes required will be generated. For a Pull Request to be valid,
format checks must not fail.

## Static code analysis

vAccel includes `clang-tidy` configs to validate it's codebase. Contributors
can run checks using the ninja target `clang-tidy` generated my meson.
Please address all errors and as much warnings as possible for the PR to be
eligible for approval. Additionally, `cppcheck` checks triggered by the Pull
Request actions must be successful.

## Contact

Feel free to contact any of the authors via commit message emails or at
<vaccel@nubificus.co.uk>.
