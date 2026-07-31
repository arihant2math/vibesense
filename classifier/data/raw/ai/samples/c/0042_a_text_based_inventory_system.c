#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ITEMS 1000
#define MAX_NAME 100

typedef struct {
    char name[MAX_NAME];
    int quantity;
} Item;

static Item inventory[MAX_ITEMS];
static int item_count = 0;

static int find_item(const char *name) {
    for (int i = 0; i < item_count; i++) {
        if (strcmp(inventory[i].name, name) == 0)
            return i;
    }
    return -1;
}

static void add_item(const char *name, int quantity) {
    if (!name || !*name || quantity <= 0)
        return;

    int index = find_item(name);

    if (index >= 0) {
        inventory[index].quantity += quantity;
        return;
    }

    if (item_count >= MAX_ITEMS)
        return;

    strncpy(inventory[item_count].name, name, MAX_NAME - 1);
    inventory[item_count].name[MAX_NAME - 1] = '\0';
    inventory[item_count].quantity = quantity;
    item_count++;
}

static void remove_item(const char *name, int quantity) {
    if (!name || !*name || quantity <= 0)
        return;

    int index = find_item(name);

    if (index < 0)
        return;

    inventory[index].quantity -= quantity;

    if (inventory[index].quantity <= 0) {
        inventory[index] = inventory[item_count - 1];
        item_count--;
    }
}

static void query_item(const char *name) {
    if (!name || !*name) {
        printf("Inventory is empty.\n");
        return;
    }

    if (strcmp(name, "all") == 0) {
        if (item_count == 0) {
            printf("Inventory is empty.\n");
            return;
        }

        for (int i = 0; i < item_count; i++)
            printf("%s %d\n", inventory[i].name, inventory[i].quantity);

        return;
    }

    int index = find_item(name);

    if (index < 0)
        printf("%s 0\n", name);
    else
        printf("%s %d\n", inventory[index].name, inventory[index].quantity);
}

int main(void) {
    char line[256];
    char command[16];
    char name[MAX_NAME];
    int quantity;

    while (fgets(line, sizeof(line), stdin)) {
        char *start = line;

        while (isspace((unsigned char)*start))
            start++;

        if (*start == '\0')
            continue;

        command[0] = '\0';
        name[0] = '\0';
        quantity = 0;

        int fields = sscanf(start, "%15s %99s %d", command, name, &quantity);

        if (strcmp(command, "add") == 0 && fields == 3) {
            add_item(name, quantity);
        } else if (strcmp(command, "remove") == 0 && fields == 3) {
            remove_item(name, quantity);
        } else if (strcmp(command, "query") == 0 && fields >= 2) {
            query_item(name);
        } else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            break;
        }
    }

    return 0;
}
