# Agent Instructions

This repository defines instructions for the Gemini CLI and any other AI agents that operate within this workspace.

**Agent Action Required:** Before executing any tasks, you must read and understand the following files:

1. `README.md`: Provides the project overview and use cases.
2. `CONTRIBUTING.md`: Contains technical details, build/test instructions, and contribution guidelines.

## Development Conventions

- **Ambiguity**: If any instructions are unclear or ambiguous, you must ask for clarification before executing any actions.

- **Git Usage**: Never commit code or add files using git unless very explicitly asked to do so by the user.

- **Surgical Changes**: Unless otherwise instructed, all changes should be consistent with the existing code style and architectural patterns. Minimize changes to only what is necessary to fulfill the request.

- **Validation**: You are responsible for ensuring your changes follow the rules in `CONTRIBUTING.md`. Always verify that changes are formatted correctly and that both CMake and Bazel tests pass.
