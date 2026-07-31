#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct TrieNode {
    struct TrieNode *children[256];
    void *value;
    int has_value;
} TrieNode;

typedef struct {
    TrieNode *root;
} Trie;

static TrieNode *trie_node_create(void) {
    return calloc(1, sizeof(TrieNode));
}

Trie *trie_create(void) {
    Trie *trie = malloc(sizeof(*trie));
    if (!trie)
        return NULL;

    trie->root = trie_node_create();
    if (!trie->root) {
        free(trie);
        return NULL;
    }

    return trie;
}

int trie_insert(Trie *trie, const unsigned char *key, void *value) {
    TrieNode *node;

    if (!trie || !trie->root || !key)
        return 0;

    node = trie->root;

    while (*key) {
        unsigned char index = *key++;

        if (!node->children[index]) {
            node->children[index] = trie_node_create();
            if (!node->children[index])
                return 0;
        }

        node = node->children[index];
    }

    node->value = value;
    node->has_value = 1;
    return 1;
}

void *trie_lookup(const Trie *trie, const unsigned char *key) {
    const TrieNode *node;

    if (!trie || !trie->root || !key)
        return NULL;

    node = trie->root;

    while (*key) {
        node = node->children[*key++];
        if (!node)
            return NULL;
    }

    return node->has_value ? node->value : NULL;
}

void *trie_longest_prefix_lookup(const Trie *trie,
                                 const unsigned char *key,
                                 size_t *matched_length) {
    const TrieNode *node;
    const TrieNode *best = NULL;
    size_t length = 0;
    size_t best_length = 0;

    if (matched_length)
        *matched_length = 0;

    if (!trie || !trie->root || !key)
        return NULL;

    node = trie->root;

    if (node->has_value) {
        best = node;
        best_length = 0;
    }

    while (*key) {
        node = node->children[*key++];
        if (!node)
            break;

        length++;

        if (node->has_value) {
            best = node;
            best_length = length;
        }
    }

    if (matched_length)
        *matched_length = best_length;

    return best ? best->value : NULL;
}

static int trie_node_empty(const TrieNode *node) {
    size_t i;

    for (i = 0; i < 256; i++) {
        if (node->children[i])
            return 0;
    }

    return !node->has_value;
}

static int trie_remove_node(TrieNode *node,
                            const unsigned char *key,
                            size_t depth,
                            size_t length,
                            void **removed_value) {
    unsigned char index;

    if (depth == length) {
        if (!node->has_value)
            return 0;

        if (removed_value)
            *removed_value = node->value;

        node->value = NULL;
        node->has_value = 0;
        return trie_node_empty(node);
    }

    index = key[depth];

    if (!node->children[index])
        return 0;

    if (trie_remove_node(node->children[index], key, depth + 1,
                         length, removed_value)) {
        free(node->children[index]);
        node->children[index] = NULL;
    }

    return trie_node_empty(node);
}

int trie_remove(Trie *trie, const unsigned char *key, void **removed_value) {
    size_t length;

    if (removed_value)
        *removed_value = NULL;

    if (!trie || !trie->root || !key)
        return 0;

    length = strlen((const char *)key);

    if (!trie->root->children[key[0]] && length != 0)
        return 0;

    trie_remove_node(trie->root, key, 0, length, removed_value);
    return removed_value ? *removed_value != NULL : 1;
}

static void trie_node_destroy(TrieNode *node, void (*free_value)(void *)) {
    size_t i;

    if (!node)
        return;

    for (i = 0; i < 256; i++)
        trie_node_destroy(node->children[i], free_value);

    if (node->has_value && free_value)
        free_value(node->value);

    free(node);
}

void trie_destroy(Trie *trie, void (*free_value)(void *)) {
    if (!trie)
        return;

    trie_node_destroy(trie->root, free_value);
    free(trie);
}
