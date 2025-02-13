#!/bin/sh
# A script to allow edits to files to be ignored without them being part of .gitignore.
# Useful for maintaining local-only config such as vscode settings.

for x in "$@" ; do
    echo Ignoring local edits to "$x"
    echo "$x" >> .git/info/exclude
done
