"""Produce a reduced compile_commands.json file with only the first compile command for each file.

Usage:
```
python scripts/reduced_compile_commands.py compile_commands.json > reduced_compile_commands.json
```
"""

import argparse
import json
import os

_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main(argv: list[str] | None = None) -> None:
    """Main entry point for script execution."""

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "input",
        help="Input compile_commands.json file",
    )
    parser.add_argument(
        "--names-only",
        help="Only output a list of unique source files",
        action="store_true",
    )
    parser.add_argument(
        "--exclude_dirs",
        help="Exclude files in any of the listed directories (relative to the project root)",
        nargs="+",
    )

    args = parser.parse_args(argv)
    args.exclude_dirs = args.exclude_dirs or []
    args.exclude_dirs = [os.path.join(_ROOT, d) for d in args.exclude_dirs]

    with open(args.input, "r") as f:
        compile_commands = json.load(f)

    compile_commands_by_file = {}
    for compile_command in compile_commands:
        file: str = compile_command["file"]
        if any(file.startswith(d) for d in args.exclude_dirs):
            continue
        if file in compile_commands:
            continue
        compile_commands_by_file[file] = compile_command

    if args.names_only:
        paths = [os.path.relpath(p, _ROOT) for p in compile_commands_by_file.keys()]
        print(" ".join(paths))
    else:
        print(json.dumps(list(compile_commands_by_file.values()), indent=2))


if __name__ == "__main__":
    main()
