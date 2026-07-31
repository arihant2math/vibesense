#!/bin/sh

FILE=${INVENTORY_FILE:-"$HOME/.inventory"}
DIR=${FILE%/*}
[ "$DIR" = "$FILE" ] && DIR=.

usage() {
    printf '%s\n' "usage: $0 add ITEM QUANTITY | remove ITEM QUANTITY | query [ITEM]" >&2
    exit 2
}

valid_item() {
    case $1 in
        ''|*[![:print:]]*|*[![:space:]]*' '*)
            return 1
            ;;
    esac
}

valid_qty() {
    case $1 in
        ''|*[!0-9]*|0)
            return 1
            ;;
    esac
}

update() {
    action=$1
    item=$2
    amount=$3
    tmp=$FILE.$$

    mkdir -p "$DIR" || exit 1

    awk -F '	' -v item="$item" -v amount="$amount" -v action="$action" '
        BEGIN { OFS = "	"; found = 0; error = 0 }
        $1 == item {
            found = 1
            qty = $2
            if (action == "add")
                qty += amount
            else
                qty -= amount
            if (qty < 0) {
                error = 1
                exit
            }
            print item, qty
            next
        }
        { print }
        END {
            if (error) exit 3
            if (!found && action == "add") print item, amount
            if (!found && action == "remove") exit 4
        }
    ' "$FILE" 2>/dev/null > "$tmp"
    status=$?

    case $status in
        0) mv "$tmp" "$FILE" ;;
        3) rm -f "$tmp"; printf '%s\n' "insufficient inventory" >&2; exit 1 ;;
        4) rm -f "$tmp"; printf '%s\n' "item not found" >&2; exit 1 ;;
        *) rm -f "$tmp"; exit 1 ;;
    esac
}

query() {
    if [ $# -eq 0 ]; then
        [ -f "$FILE" ] && sort "$FILE"
    else
        awk -F '	' -v item="$1" '$1 == item { print $1 "	" $2; found = 1 } END { if (!found) exit 1 }' "$FILE" 2>/dev/null ||
            printf '%s\n' "item not found" >&2
    fi
}

[ $# -ge 1 ] || usage

case $1 in
    add|remove)
        [ $# -eq 3 ] || usage
        valid_item "$2" || { printf '%s\n' "invalid item" >&2; exit 2; }
        valid_qty "$3" || { printf '%s\n' "invalid quantity" >&2; exit 2; }
        update "$1" "$2" "$3"
        ;;
    query)
        [ $# -le 2 ] || usage
        query "${2-}"
        ;;
    *)
        usage
        ;;
esac
