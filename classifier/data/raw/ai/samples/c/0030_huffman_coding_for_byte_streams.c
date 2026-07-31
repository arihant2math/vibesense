#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HUFFMAN_SYMBOLS 256
#define HUFFMAN_HEADER_SIZE (HUFFMAN_SYMBOLS * sizeof(uint64_t))

typedef struct {
    uint64_t frequency;
    int left;
    int right;
    int symbol;
} HuffmanNode;

typedef struct {
    HuffmanNode nodes[HUFFMAN_SYMBOLS * 2];
    int count;
    int root;
} HuffmanTree;

static void write_u64_le(uint8_t *output, uint64_t value)
{
    for (int i = 0; i < 8; i++) {
        output[i] = (uint8_t)(value & 0xff);
        value >>= 8;
    }
}

static uint64_t read_u64_le(const uint8_t *input)
{
    uint64_t value = 0;

    for (int i = 7; i >= 0; i--) {
        value <<= 8;
        value |= input[i];
    }

    return value;
}

static int build_tree(const uint64_t frequencies[HUFFMAN_SYMBOLS],
                      HuffmanTree *tree)
{
    int active[HUFFMAN_SYMBOLS * 2];
    int active_count = 0;

    tree->count = 0;
    tree->root = -1;

    for (int i = 0; i < HUFFMAN_SYMBOLS; i++) {
        if (frequencies[i] != 0) {
            HuffmanNode *node = &tree->nodes[tree->count];

            node->frequency = frequencies[i];
            node->left = -1;
            node->right = -1;
            node->symbol = i;

            active[active_count++] = tree->count++;
        }
    }

    if (active_count == 0) {
        return 1;
    }

    if (active_count == 1) {
        tree->root = active[0];
        return 1;
    }

    while (active_count > 1) {
        int first = -1;
        int second = -1;

        for (int i = 0; i < active_count; i++) {
            int candidate = active[i];

            if (first == -1 ||
                tree->nodes[candidate].frequency <
                    tree->nodes[first].frequency) {
                second = first;
                first = candidate;
            } else if (second == -1 ||
                       tree->nodes[candidate].frequency <
                           tree->nodes[second].frequency) {
                second = candidate;
            }
        }

        if (first == -1 || second == -1 ||
            tree->nodes[first].frequency >
                UINT64_MAX - tree->nodes[second].frequency) {
            return 0;
        }

        HuffmanNode *parent = &tree->nodes[tree->count];

        parent->frequency =
            tree->nodes[first].frequency + tree->nodes[second].frequency;
        parent->left = first;
        parent->right = second;
        parent->symbol = -1;

        int write_index = 0;

        for (int i = 0; i < active_count; i++) {
            if (active[i] != first && active[i] != second) {
                active[write_index++] = active[i];
            }
        }

        active[write_index++] = tree->count++;
        active_count = write_index;
    }

    tree->root = active[0];
    return 1;
}

static void build_codes(const HuffmanTree *tree,
                        int node_index,
                        uint8_t codes[HUFFMAN_SYMBOLS][HUFFMAN_SYMBOLS],
                        uint16_t lengths[HUFFMAN_SYMBOLS],
                        uint8_t path[HUFFMAN_SYMBOLS],
                        uint16_t depth)
{
    const HuffmanNode *node = &tree->nodes[node_index];

    if (node->symbol >= 0) {
        if (depth == 0) {
            path[0] = 0;
            depth = 1;
        }

        memcpy(codes[node->symbol], path, depth);
        lengths[node->symbol] = depth;
        return;
    }

    path[depth] = 0;
    build_codes(tree, node->left, codes, lengths, path, depth + 1);

    path[depth] = 1;
    build_codes(tree, node->right, codes, lengths, path, depth + 1);
}

int huffman_compress(const uint8_t *input,
                     size_t input_size,
                     uint8_t **output,
                     size_t *output_size)
{
    uint64_t frequencies[HUFFMAN_SYMBOLS] = {0};
    HuffmanTree tree;
    uint8_t codes[HUFFMAN_SYMBOLS][HUFFMAN_SYMBOLS] = {{0}};
    uint16_t lengths[HUFFMAN_SYMBOLS] = {0};
    uint8_t path[HUFFMAN_SYMBOLS];
    uint64_t bit_count = 0;
    size_t data_size;
    size_t total_size;
    uint8_t *result;
    size_t bit_position = 0;

    if (!output || !output_size || (input_size != 0 && !input)) {
        return 0;
    }

    for (size_t i = 0; i < input_size; i++) {
        if (frequencies[input[i]] == UINT64_MAX) {
            return 0;
        }

        frequencies[input[i]]++;
    }

    if (!build_tree(frequencies, &tree)) {
        return 0;
    }

    if (tree.root != -1) {
        build_codes(&tree, tree.root, codes, lengths, path, 0);
    }

    for (int i = 0; i < HUFFMAN_SYMBOLS; i++) {
        if (frequencies[i] != 0 &&
            lengths[i] > UINT64_MAX / frequencies[i]) {
            return 0;
        }

        if (frequencies[i] != 0) {
            uint64_t symbol_bits = frequencies[i] * lengths[i];

            if (bit_count > UINT64_MAX - symbol_bits) {
                return 0;
            }

            bit_count += symbol_bits;
        }
    }

    if (bit_count > (uint64_t)SIZE_MAX - 7) {
        return 0;
    }

    data_size = (size_t)((bit_count + 7) / 8);

    if (HUFFMAN_HEADER_SIZE > SIZE_MAX - data_size) {
        return 0;
    }

    total_size = HUFFMAN_HEADER_SIZE + data_size;
    result = (uint8_t *)calloc(total_size ? total_size : 1, 1);

    if (!result) {
        return 0;
    }

    for (int i = 0; i < HUFFMAN_SYMBOLS; i++) {
        write_u64_le(result + i * 8, frequencies[i]);
    }

    for (size_t i = 0; i < input_size; i++) {
        uint8_t symbol = input[i];

        for (uint16_t j = 0; j < lengths[symbol]; j++) {
            if (codes[symbol][j]) {
                result[HUFFMAN_HEADER_SIZE + bit_position / 8] |=
                    (uint8_t)(1u << (7 - (bit_position % 8)));
            }

            bit_position++;
        }
    }

    *output = result;
    *output_size = total_size;
    return 1;
}

int huffman_decompress(const uint8_t *input,
                       size_t input_size,
                       uint8_t **output,
                       size_t *output_size)
{
    uint64_t frequencies[HUFFMAN_SYMBOLS];
    HuffmanTree tree;
    size_t expected_size = 0;
    size_t data_size;
    uint8_t *result;
    size_t result_position = 0;
    size_t bit_position = 0;

    if (!output || !output_size ||
        input_size < HUFFMAN_HEADER_SIZE ||
        (input_size != 0 && !input)) {
        return 0;
    }

    for (int i = 0; i < HUFFMAN_SYMBOLS; i++) {
        frequencies[i] = read_u64_le(input + i * 8);

        if (frequencies[i] > SIZE_MAX - expected_size) {
            return 0;
        }

        expected_size += (size_t)frequencies[i];
    }

    if (!build_tree(frequencies, &tree)) {
        return 0;
    }

    data_size = input_size - HUFFMAN_HEADER_SIZE;

    result = (uint8_t *)malloc(expected_size ? expected_size : 1);

    if (!result) {
        return 0;
    }

    if (expected_size == 0) {
        *output = result;
        *output_size = 0;
        return 1;
    }

    if (tree.root < 0) {
        free(result);
        return 0;
    }

    if (tree.nodes[tree.root].symbol >= 0) {
        for (size_t i = 0; i < expected_size; i++) {
            result[i] = (uint8_t)tree.nodes[tree.root].symbol;
        }

        *output = result;
        *output_size = expected_size;
        return 1;
    }

    while (result_position < expected_size &&
           bit_position < data_size * 8) {
        int node_index = tree.root;

        while (tree.nodes[node_index].symbol < 0) {
            size_t byte_index = bit_position / 8;
            unsigned bit_index = 7u - (unsigned)(bit_position % 8);
            unsigned bit;

            if (byte_index >= data_size) {
                free(result);
                return 0;
            }

            bit = (input[HUFFMAN_HEADER_SIZE + byte_index] >> bit_index) & 1u;
            bit_position++;

            node_index = bit ? tree.nodes[node_index].right
                             : tree.nodes[node_index].left;

            if (node_index < 0) {
                free(result);
                return 0;
            }
        }

        result[result_position++] = (uint8_t)tree.nodes[node_index].symbol;
    }

    if (result_position != expected_size) {
        free(result);
        return 0;
    }

    *output = result;
    *output_size = expected_size;
    return 1;
}
