#!/bin/sh

if [ "$#" -lt 2 ]; then
    exit 1
fi

target=$1
shift

get_element() {
    index=$1
    shift
    current_index=0

    for value do
        if [ "$current_index" -eq "$index" ]; then
            printf '%s\n' "$value"
            return
        fi
        current_index=$((current_index + 1))
    done
}

array_size=$#
left=0
right=$((array_size - 1))
result=-1

while [ "$left" -le "$right" ]; do
    middle=$((left + (right - left) / 2))

    middle_value=$(get_element "$middle" "$@")
    left_value=$(get_element "$left" "$@")
    right_value=$(get_element "$right" "$@")

    if [ "$middle_value" -eq "$target" ]; then
        result=$middle
        break
    fi

    if [ "$left_value" -eq "$middle_value" ] &&
       [ "$middle_value" -eq "$right_value" ]; then
        left=$((left + 1))
        right=$((right - 1))
    elif [ "$left_value" -le "$middle_value" ]; then
        if [ "$left_value" -le "$target" ] &&
           [ "$target" -lt "$middle_value" ]; then
            right=$((middle - 1))
        else
            left=$((middle + 1))
        fi
    else
        if [ "$middle_value" -lt "$target" ] &&
           [ "$target" -le "$right_value" ]; then
            left=$((middle + 1))
        else
            right=$((middle - 1))
        fi
    fi
done

printf '%s\n' "$result"
