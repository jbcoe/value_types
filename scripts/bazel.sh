#!/bin/bash

set -eux -o pipefail

bazel test //...
