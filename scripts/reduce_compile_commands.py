"""Produce a reduced compile_commands.json file with only the first compile command for each file.

Usage:
```
python scripts/reduced_compile_commands.py compile_commands.json > reduced_compile_commands.json
```
"""

import argparse
import json


def main(argv: list[str] | None = None) -> None:
    """Main entry point for script execution."""

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "input",
        help="Input compile_commands.json file",
    )

    args = parser.parse_args(argv)

    with open(args.input, "r") as f:
        compile_commands = json.load(f)

    compile_commands_by_file = {}
    for compile_command in compile_commands:
        file = compile_command["file"]
        if "/_deps/" in file:
            continue
        if file in compile_commands:
            continue
        compile_commands_by_file[file] = compile_command

    print(json.dumps(list(compile_commands_by_file.values()), indent=2))


if __name__ == "__main__":
    main()
