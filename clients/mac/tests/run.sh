#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
cc -std=c89 -pedantic -Wall -Wextra -Werror -Itests \
    wire.c network.c tests/check.c -o "$test_dir/check"
"$test_dir/check"
