#!/bin/sh

NC=${NC:-nc}
HOST=${HOST:-127.0.0.1}
PORT=${PORT:-9999}
INTERVAL=${INTERVAL:-5}
TIMEOUT=${TIMEOUT:-1}
HEARTBEAT_ID=${HEARTBEAT_ID:-$$}
CLOCK=${CLOCK:-date}

now() {
    "$CLOCK" +%s
}

send_heartbeat() {
    message="HEARTBEAT $HEARTBEAT_ID $(now)"
    printf '%s\n' "$message" |
        "$NC" -u -w "$TIMEOUT" "$HOST" "$PORT" >/dev/null 2>&1
}

run_sender() {
    while :; do
        send_heartbeat
        sleep "$INTERVAL"
    done
}

valid_heartbeat() {
    packet=$1
    old_glob=$(set +f)
    set -f
    set -- $packet
    eval "$old_glob"

    [ "$#" -eq 3 ] &&
    [ "$1" = HEARTBEAT ] &&
    [ -n "$2" ] &&
    case $3 in
        ''|*[!0-9]*) return 1 ;;
    esac
}

receive_heartbeat() {
    packet=$("$NC" -u -l -p "$PORT" -w "$TIMEOUT" 2>/dev/null)

    [ -n "$packet" ] || return 1

    if valid_heartbeat "$packet"; then
        printf 'OK %s\n' "$packet"
        return 0
    fi

    printf 'INVALID %s\n' "$packet" >&2
    return 1
}

run_receiver() {
    while :; do
        receive_heartbeat || :
    done
}

usage() {
    printf 'usage: %s sender|receiver\n' "$0" >&2
    exit 2
}

[ "$#" -eq 1 ] || usage

case $1 in
    sender) run_sender ;;
    receiver) run_receiver ;;
    *) usage ;;
esac
