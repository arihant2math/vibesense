#include <stdio.h>
#include <stdbool.h>

bool regex_match(const char *s, const char *p) {
    if (*p == '\0')
        return *s == '\0';

    bool first_match = (*s != '\0' && (*p == *s || *p == '.'));

    if (p[1] == '*')
        return regex_match(s, p + 2) ||
               (first_match && regex_match(s + 1, p));

    return first_match && regex_match(s + 1, p + 1);
}

int main(void) {
    char s[1024], p[1024];

    if (scanf("%1023s %1023s", s, p) != 2)
        return 1;

    printf("%s\n", regex_match(s, p) ? "true" : "false");
    return 0;
}
