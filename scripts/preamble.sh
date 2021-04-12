#!/bin/bash

# Sets common flags to catch script bugs
set -o nounset
set -o errexit
set -o pipefail

SCRIPTPATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
export SCRIPTPATH

WORKSPACE_DIR="$SCRIPTPATH/.."
export WORKSPACE_DIR
