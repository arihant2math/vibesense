#!/bin/sh

graph=$1
source=$2

[ -n "$graph" ] && [ -n "$source" ] || {
    printf 'usage: %s graph-file source\n' "$0" >&2
    exit 2
}

awk -v src="$source" '
BEGIN {
    OFS = " "
    n = 0
    dist[src] = 0
    vertex[src] = 1
    order[++n] = src
}
NF >= 3 && $1 !~ /^#/ {
    u = $1
    v = $2
    w = $3 + 0

    vertex[u] = 1
    vertex[v] = 1

    if (!(u in index))
        order[++n] = u
    if (!(v in index))
        order[++n] = v
    index[u] = 1
    index[v] = 1

    edge_count[u]++
    e = edge_count[u]
    to[u, e] = v
    weight[u, e] = w
}
END {
    for (i = 1; i <= n; i++) {
        u = order[i]
        if (!(u in dist))
            dist[u] = 1e308
    }

    for (step = 1; step <= n; step++) {
        u = ""
        best = 1e308

        for (i = 1; i <= n; i++) {
            v = order[i]
            if (!done[v] && dist[v] < best) {
                best = dist[v]
                u = v
            }
        }

        if (u == "")
            break

        done[u] = 1

        for (i = 1; i <= edge_count[u]; i++) {
            v = to[u, i]
            alt = dist[u] + weight[u, i]
            if (alt < dist[v])
                dist[v] = alt
        }
    }

    for (i = 1; i <= n; i++) {
        u = order[i]
        if (dist[u] >= 1e308)
            print u, "INF"
        else
            print u, dist[u]
    }
}
' "$graph"
