#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    int major;
    int minor;
    int patch;
    const char *prerelease;
} SemVer;

static int compare_identifiers(const char *a, const char *b)
{
    int a_numeric = 1, b_numeric = 1;
    const char *p;

    for (p = a; *p && *p != '.'; ++p)
        if (!isdigit((unsigned char)*p)) a_numeric = 0;

    for (p = b; *p && *p != '.'; ++p)
        if (!isdigit((unsigned char)*p)) b_numeric = 0;

    if (a_numeric && b_numeric) {
        long av = strtol(a, NULL, 10);
        long bv = strtol(b, NULL, 10);
        return (av > bv) - (av < bv);
    }

    if (a_numeric != b_numeric)
        return a_numeric ? -1 : 1;

    return strcmp(a, b);
}

static int compare_prerelease(const char *a, const char *b)
{
    if ((!a || !*a) && (!b || !*b)) return 0;
    if (!a || !*a) return 1;
    if (!b || !*b) return -1;

    while (*a || *b) {
        char ai[64] = {0};
        char bi[64] = {0};
        size_t i = 0, j = 0;
        int result;

        while (*a && *a != '.' && i < sizeof(ai) - 1)
            ai[i++] = *a++;
        while (*b && *b != '.' && j < sizeof(bi) - 1)
            bi[j++] = *b++;

        if (!i && !j) break;
        if (!i) return -1;
        if (!j) return 1;

        result = compare_identifiers(ai, bi);
        if (result) return result;

        if (*a == '.') ++a;
        if (*b == '.') ++b;
    }

    if (!*a && !*b) return 0;
    return *a ? 1 : -1;
}

int semver_compare(SemVer a, SemVer b)
{
    if (a.major != b.major) return (a.major > b.major) - (a.major < b.major);
    if (a.minor != b.minor) return (a.minor > b.minor) - (a.minor < b.minor);
    if (a.patch != b.patch) return (a.patch > b.patch) - (a.patch < b.patch);
    return compare_prerelease(a.prerelease, b.prerelease);
}

int semver_equal(SemVer a, SemVer b) { return semver_compare(a, b) == 0; }
int semver_less(SemVer a, SemVer b) { return semver_compare(a, b) < 0; }
int semver_less_equal(SemVer a, SemVer b) { return semver_compare(a, b) <= 0; }
int semver_greater(SemVer a, SemVer b) { return semver_compare(a, b) > 0; }
int semver_greater_equal(SemVer a, SemVer b) { return semver_compare(a, b) >= 0; }

int main(void)
{
    SemVer a = {1, 2, 3, "beta.2"};
    SemVer b = {1, 2, 3, "beta.11"};

    printf("a %s b\n",
           semver_less(a, b) ? "<" :
           semver_equal(a, b) ? "==" : ">");

    printf("a == b: %s\n", semver_equal(a, b) ? "true" : "false");
    printf("a <  b: %s\n", semver_less(a, b) ? "true" : "false");
    printf("a >= b: %s\n", semver_greater_equal(a, b) ? "true" : "false");

    return 0;
}
