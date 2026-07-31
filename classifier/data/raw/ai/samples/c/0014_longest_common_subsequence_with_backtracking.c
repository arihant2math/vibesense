#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} Result;

static void append_char(Result *result, char c) {
    if (result->length + 1 >= result->capacity) {
        result->capacity = result->capacity ? result->capacity * 2 : 16;
        result->data = realloc(result->data, result->capacity);
    }
    result->data[result->length++] = c;
    result->data[result->length] = '\0';
}

static void forward_scores(const char *a, size_t m,
                           const char *b, size_t n,
                           size_t *scores) {
    memset(scores, 0, (n + 1) * sizeof(*scores));

    for (size_t i = 0; i < m; ++i) {
        size_t diagonal = 0;
        for (size_t j = 1; j <= n; ++j) {
            size_t above = scores[j];
            if (a[i] == b[j - 1])
                scores[j] = diagonal + 1;
            else if (scores[j - 1] > scores[j])
                scores[j] = scores[j - 1];
            diagonal = above;
        }
    }
}

static void backward_scores(const char *a, size_t m,
                            const char *b, size_t n,
                            size_t *scores) {
    size_t *next = calloc(n + 1, sizeof(*next));
    size_t *current = calloc(n + 1, sizeof(*current));

    for (size_t i = m; i-- > 0;) {
        current[n] = 0;
        for (size_t j = n; j-- > 0;) {
            if (a[i] == b[j])
                current[j] = next[j + 1] + 1;
            else if (next[j] > current[j + 1])
                current[j] = next[j];
            else
                current[j] = current[j + 1];
        }

        size_t *tmp = next;
        next = current;
        current = tmp;
    }

    memcpy(scores, next, (n + 1) * sizeof(*scores));
    free(next);
    free(current);
}

static void hirschberg(const char *a, size_t m,
                       const char *b, size_t n,
                       Result *result) {
    if (m == 0 || n == 0)
        return;

    if (m == 1) {
        for (size_t j = 0; j < n; ++j) {
            if (a[0] == b[j]) {
                append_char(result, a[0]);
                break;
            }
        }
        return;
    }

    size_t mid = m / 2;
    size_t *left = calloc(n + 1, sizeof(*left));
    size_t *right = calloc(n + 1, sizeof(*right));

    forward_scores(a, mid, b, n, left);
    backward_scores(a + mid, m - mid, b, n, right);

    size_t split = 0;
    size_t best = 0;

    for (size_t j = 0; j <= n; ++j) {
        size_t score = left[j] + right[j];
        if (score > best) {
            best = score;
            split = j;
        }
    }

    free(left);
    free(right);

    hirschberg(a, mid, b, split, result);
    hirschberg(a + mid, m - mid, b + split, n - split, result);
}

int main(int argc, char **argv) {
    if (argc != 3)
        return 1;

    const char *a = argv[1];
    const char *b = argv[2];
    size_t m = strlen(a);
    size_t n = strlen(b);

    if (n > m) {
        const char *tmp = a;
        a = b;
        b = tmp;

        size_t length = m;
        m = n;
        n = length;
    }

    Result result = {0};
    hirschberg(a, m, b, n, &result);

    if (result.data)
        fputs(result.data, stdout);

    free(result.data);
    return 0;
}
