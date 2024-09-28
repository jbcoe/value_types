#!/bin/sh

set -eux -o pipefail

bazel test //...
