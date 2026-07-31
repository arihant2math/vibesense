#!/bin/sh

retry_with_backoff() {
    max_attempts=$1
    shift

    attempt=1
    delay=1

    while [ "$attempt" -le "$max_attempts" ]; do
        "$@" && return 0

        if [ "$attempt" -eq "$max_attempts" ]; then
            break
        fi

        jitter=$(( $(od -An -N2 -tu2 /dev/urandom) % delay + 1 ))
        sleep $((delay + jitter))

        delay=$((delay * 2))
        attempt=$((attempt + 1))
    done

    return 1
}

# Example:
# retry_with_backoff 5 curl -fsS https://example.com
