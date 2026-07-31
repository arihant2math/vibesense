#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define ALPHABET_SIZE 256

typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    bool is_word;
} TrieNode;

TrieNode *trie_create_node(void) {
    return calloc(1, sizeof(TrieNode));
}

TrieNode *trie_create(void) {
    return trie_create_node();
}

void trie_free(TrieNode *node) {
    if (node == NULL) {
        return;
    }

    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        trie_free(node->children[i]);
    }

    free(node);
}

bool trie_insert(TrieNode *root, const char *word) {
    if (root == NULL || word == NULL) {
        return false;
    }

    TrieNode *current = root;

    for (const unsigned char *p = (const unsigned char *)word; *p != '\0'; ++p) {
        if (current->children[*p] == NULL) {
            current->children[*p] = trie_create_node();
            if (current->children[*p] == NULL) {
                return false;
            }
        }

        current = current->children[*p];
    }

    current->is_word = true;
    return true;
}

bool trie_contains(const TrieNode *root, const char *word) {
    if (root == NULL || word == NULL) {
        return false;
    }

    const TrieNode *current = root;

    for (const unsigned char *p = (const unsigned char *)word; *p != '\0'; ++p) {
        current = current->children[*p];
        if (current == NULL) {
            return false;
        }
    }

    return current->is_word;
}

bool trie_has_prefix(const TrieNode *root, const char *prefix) {
    if (root == NULL || prefix == NULL) {
        return false;
    }

    const TrieNode *current = root;

    for (const unsigned char *p = (const unsigned char *)prefix; *p != '\0'; ++p) {
        current = current->children[*p];
        if (current == NULL) {
            return false;
        }
    }

    return true;
}

bool trie_remove(TrieNode *node, const char *word, size_t depth) {
    if (node == NULL || word == NULL) {
        return false;
    }

    if (word[depth] != '\0') {
        unsigned char index = (unsigned char)word[depth];

        if (node->children[index] == NULL ||
            !trie_remove(node->children[index], word, depth + 1)) {
            return false;
        }

        TrieNode *child = node->children[index];
        bool child_empty = !child->is_word;

        for (size_t i = 0; i < ALPHABET_SIZE && child_empty; ++i) {
            child_empty = child->children[i] == NULL;
        }

        if (child_empty) {
            free(child);
            node->children[index] = NULL;
        }

        return true;
    }

    if (!node->is_word) {
        return false;
    }

    node->is_word = false;
    return true;
}

int main(void) {
    TrieNode *trie = trie_create();
    if (trie == NULL) {
        return EXIT_FAILURE;
    }

    const char *words[] = {"app", "apple", "application", "bat"};
    size_t word_count = sizeof(words) / sizeof(words[0]);

    for (size_t i = 0; i < word_count; ++i) {
        if (!trie_insert(trie, words[i])) {
            trie_free(trie);
            return EXIT_FAILURE;
        }
    }

    printf("%s\n", trie_has_prefix(trie, "") ? "true" : "false");
    printf("%s\n", trie_has_prefix(trie, "app") ? "true" : "false");
    printf("%s\n", trie_contains(trie, "app") ? "true" : "false");
    printf("%s\n", trie_contains(trie, "ap") ? "true" : "false");

    trie_remove(trie, "app", 0);
    printf("%s\n", trie_contains(trie, "app") ? "true" : "false");
    printf("%s\n", trie_contains(trie, "apple") ? "true" : "false");

    trie_free(trie);
    return EXIT_SUCCESS;
}
