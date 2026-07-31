#!/bin/sh

LC_ALL=C
export LC_ALL

if [ "$#" -ne 1 ] || [ -z "$1" ]; then
    exit 1
fi

set -f
set -- $(printf '%s' "$1" | od -An -v -tu1 | awk '{ for (i = 1; i <= NF; i++) print $i }')

[ "$#" -gt 0 ] || exit 1

od -An -v -tu1 | awk '{ for (i = 1; i <= NF; i++) print $i }' |
while IFS= read -r byte; do
    key=$1
    value=$((byte ^ key))
    printf "\\$(printf '%03o' "$value")"

    shift
    set -- "$@" "$key"
done
