#!/bin/bash
#shellcheck source=./preamble.sh
source "${BASH_SOURCE%/*}/./preamble.sh"

cp -R "$WORKSPACE_DIR/firmware/libraries/EverydayCalendar" ~/Documents/Arduino/libraries
cp -R "$WORKSPACE_DIR/firmware/libraries/IQS5xx" ~/Documents/Arduino/libraries
cp -R "$WORKSPACE_DIR/firmware/libraries/Wire_BigBuffer" ~/Documents/Arduino/libraries