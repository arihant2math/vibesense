#!/bin/sh

awk '
BEGIN {
    max_level = 32
    count = 0
    head = 0
    next_node = 1
    srand()
}

function random_level( level) {
    level = 1
    while (level < max_level && rand() < 0.5)
        level++
    return level
}

function find(key, update,    x, i) {
    x = head
    for (i = max_level; i >= 1; i--) {
        while (next_node[i SUBSEP x] != "" &&
               key_of[next_node[i SUBSEP x]] < key)
            x = next_node[i SUBSEP x]
        update[i] = x
    }
    return next_node[1 SUBSEP x]
}

function insert(key, value,    update, x, level, n, i) {
    x = find(key, update)

    if (x != "" && key_of[x] == key) {
        value_of[x] = value
        return
    }

    if (count >= 2147483647)
        exit 75

    level = random_level()
    n = next_node
    if (n >= 2147483647)
        exit 75

    next_node++
    key_of[n] = key
    value_of[n] = value
    level_of[n] = level

    for (i = 1; i <= level; i++) {
        node_next[i SUBSEP n] = next_node[i SUBSEP update[i]]
        next_node[i SUBSEP update[i]] = n
    }

    count++
}

function remove_key(key,    update, x, i) {
    x = find(key, update)

    if (x == "" || key_of[x] != key)
        return 1

    for (i = 1; i <= level_of[x]; i++) {
        if (next_node[i SUBSEP update[i]] == x)
            next_node[i SUBSEP update[i]] = node_next[i SUBSEP x]
    }

    delete key_of[x]
    delete value_of[x]
    delete level_of[x]

    for (i = 1; i <= max_level; i++)
        delete node_next[i SUBSEP x]

    if (count > 0)
        count--

    return 0
}

function search(key,    x) {
    x = find(key)
    if (x != "" && key_of[x] == key)
        print value_of[x]
    else
        print "NOT_FOUND"
}

function dump(    x) {
    x = next_node[1 SUBSEP head]
    while (x != "") {
        print key_of[x] "\t" value_of[x]
        x = node_next[1 SUBSEP x]
    }
}

$1 == "insert" && NF >= 3 {
    key = $2
    value = $0
    sub(/^insert[[:space:]]+[^[:space:]]+[[:space:]]*/, "", value)
    insert(key, value)
    next
}

$1 == "search" && NF >= 2 {
    search($2)
    next
}

$1 == "delete" && NF >= 2 {
    if (remove_key($2) != 0)
        print "NOT_FOUND"
    next
}

$1 == "dump" {
    dump()
    next
}

$1 == "count" {
    print count
    next
}

$1 == "quit" || $1 == "exit" {
    exit
}

' "$@"
