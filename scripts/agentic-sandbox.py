#!/usr/bin/env python3
"""
Script to run an AI agent in a Docker sandbox.

Supported agents: Claude Code, Antigravity CLI.
"""

import argparse
import os
import subprocess
import sys


def _seed_config_file(path: str, content: bytes) -> None:
    """
    Create path with content and mode 0o600.

    Skips silently if it already exists.
    """
    try:
        fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    except FileExistsError:
        return
    try:
        os.write(fd, content)
    finally:
        os.close(fd)


def main() -> None:
    """Provide the main entry point for the agentic sandbox script."""
    parser = argparse.ArgumentParser(
        description="Run an AI agent (claude or antigravity)"
        " in a Docker sandbox."
    )
    parser.add_argument(
        "agent",
        choices=["claude", "antigravity"],
        help="AI agent to run inside the sandbox.",
    )
    parser.add_argument(
        "--update",
        action="store_true",
        help="Update the agent CLI inside the container before running.",
    )
    parser.add_argument(
        "--rebuild-docker", action="store_true", help="Rebuild the Docker image."
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Enable verbose logging."
    )

    args = parser.parse_args()

    def log(msg: str) -> None:
        """Log a message if verbose mode is enabled."""
        if args.verbose:
            print(msg)

    project_root = subprocess.check_output(
        ["git", "rev-parse", "--show-toplevel"], text=True
    ).strip()

    image_name = "cc-protocol-sandbox"

    image_exists = (
        subprocess.run(
            ["docker", "image", "inspect", image_name],
            capture_output=True,
        ).returncode
        == 0
    )

    if args.rebuild_docker or not image_exists:
        log(f"--- Building Docker Sandbox: {image_name} ---")
        subprocess.check_call(
            [
                "docker",
                "build",
                "-t",
                image_name,
                "-f",
                os.path.join(project_root, "docker/Dockerfile"),
                project_root,
            ]
        )

    log(f"--- Starting Sandboxed {args.agent.capitalize()} Session ---")
    log(f"Note: Your current directory {project_root} is mounted to /workspace")

    if args.agent == "antigravity":
        npm_package = None
        agent_cmd = "agy"
    else:
        npm_package = "@anthropic-ai/claude-code"
        agent_cmd = "claude --dangerously-skip-permissions"

    if args.update:
        if args.agent == "antigravity":
            container_cmd = (
                "curl -fsSL https://antigravity.google/cli/install.sh | bash && "
                f"{agent_cmd}"
            )
        else:
            container_cmd = (
                f"export NPM_CONFIG_PREFIX=~/.npm-global && "
                f"export PATH=~/.npm-global/bin:$PATH && "
                f"npm install -g {npm_package}@latest && {agent_cmd}"
            )
    else:
        container_cmd = agent_cmd

    run_args = [
        "docker",
        "run",
        "-it",
        "--rm",
        "-v",
        f"{project_root}:/workspace",
    ]

    if args.agent == "antigravity":
        antigravity_config_dir = os.path.expanduser("~/.gemini")
        os.makedirs(antigravity_config_dir, mode=0o700, exist_ok=True)
        # Seed default config files if missing so they are accessible inside the
        # container.
        # Use os.open with restrictive permissions to avoid exposing credentials to
        # other local users on multi-user systems.
        _seed_config_file(
            os.path.join(antigravity_config_dir, "trustedFolders.json"),
            b'{"/workspace": "TRUST_FOLDER"}',
        )
        _seed_config_file(
            os.path.join(antigravity_config_dir, "settings.json"),
            b'{"selectedAuthType": "oauth-personal"}',
        )
        run_args.extend(["-v", f"{antigravity_config_dir}:/home/vscode/.gemini"])
    else:
        host_claude_dir = os.path.expanduser("~/.claude")
        host_claude_json = os.path.expanduser("~/.claude.json")
        os.makedirs(host_claude_dir, mode=0o700, exist_ok=True)
        # Ensure the file exists on the host so Docker doesn't create it as a directory.
        # Use os.open with restrictive permissions to avoid exposing credentials to
        # other local users on multi-user systems.
        if os.path.exists(host_claude_json):
            if not os.path.isfile(host_claude_json):
                sys.exit(
                    f"Expected {host_claude_json} to be a regular file, but found a "
                    "different filesystem object. Remove or rename it and rerun."
                )
        else:
            _seed_config_file(host_claude_json, b"")
        run_args.extend(["-v", f"{host_claude_dir}:/home/vscode/.claude"])
        run_args.extend(["-v", f"{host_claude_json}:/home/vscode/.claude.json"])

    if "TERM" in os.environ:
        run_args.extend(["-e", f"TERM={os.environ['TERM']}"])
    if "COLORTERM" in os.environ:
        run_args.extend(["-e", f"COLORTERM={os.environ['COLORTERM']}"])

    run_args.extend([image_name, "bash", "-c", container_cmd])

    subprocess.run(run_args, check=True)


if __name__ == "__main__":
    main()
