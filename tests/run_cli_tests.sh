#!/usr/bin/env bash
# Tests of the command line of hw3d.
#   data/NAME.in        a scene
#   data/NAME.out       what "hw3d --list" must print for it (the numbers of the intersecting triangles)
#   data/NAME.fail.in   a broken input: the program must print an error and exit with the code 1
# Every case is run twice: the scene is a file in the command line, and the scene comes from the standard input.
# usage: tests/run_cli_tests.sh path/to/hw3d

BIN=${1:?usage: run_cli_tests.sh path/to/hw3d}
DIR="$(cd "$(dirname "$0")" && pwd)/data"
failed=0
total=0

for input in "$DIR"/*.in; do
    name=$(basename "$input" .in)
    # checking for reading data from a file and from the console
    for mode in file stdin; do
        total=$((total + 1))
        if [[ $mode == file ]]; then
            output=$("$BIN" --list "$input" 2>/dev/null); code=$?
        else
            output=$("$BIN" --list < "$input" 2>/dev/null); code=$?
        fi

        if [[ $name == *.fail ]]; then
            if [[ $code -eq 1 && -z $output ]]; then
                echo "ok    $name ($mode): an error, exit code 1"
            else
                echo "FAIL  $name ($mode): exit code $code, output '$output'"; failed=$((failed + 1))
            fi
        else
            expected=$(cat "$DIR/$name.out")
            if [[ $code -eq 0 && $output == "$expected" ]]; then
                echo "ok    $name ($mode)"
            else
                echo "FAIL  $name ($mode): exit code $code"
                echo "      expected: $(echo $expected)"
                echo "      got:      $(echo $output)"
                failed=$((failed + 1))
            fi
        fi
    done
done

echo "cli tests: $((total - failed)) of $total passed"
[[ $failed -eq 0 ]]
