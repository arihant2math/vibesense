#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct PropertyChangeEvent {
    const char *property_name;
    const void *old_value;
    const void *new_value;
} PropertyChangeEvent;

typedef void (*PropertyChangeCallback)(
    void *observer,
    const PropertyChangeEvent *event
);

typedef struct Observer {
    void *context;
    PropertyChangeCallback callback;
    struct Observer *next;
} Observer;

typedef struct {
    char *name;
    int age;
    Observer *observers;
} Person;

static void notify_observers(
    Person *person,
    const char *property_name,
    const void *old_value,
    const void *new_value
) {
    PropertyChangeEvent event = {
        .property_name = property_name,
        .old_value = old_value,
        .new_value = new_value
    };

    for (Observer *observer = person->observers;
         observer != NULL;
         observer = observer->next) {
        observer->callback(observer->context, &event);
    }
}

void person_init(Person *person, const char *name, int age) {
    person->name = malloc(strlen(name) + 1);
    if (person->name != NULL) {
        strcpy(person->name, name);
    }

    person->age = age;
    person->observers = NULL;
}

void person_destroy(Person *person) {
    Observer *observer = person->observers;

    while (observer != NULL) {
        Observer *next = observer->next;
        free(observer);
        observer = next;
    }

    free(person->name);
    person->name = NULL;
    person->observers = NULL;
}

int person_add_observer(
    Person *person,
    void *context,
    PropertyChangeCallback callback
) {
    if (person == NULL || callback == NULL) {
        return 0;
    }

    Observer *observer = malloc(sizeof(*observer));
    if (observer == NULL) {
        return 0;
    }

    observer->context = context;
    observer->callback = callback;
    observer->next = person->observers;
    person->observers = observer;

    return 1;
}

void person_remove_observer(
    Person *person,
    void *context,
    PropertyChangeCallback callback
) {
    if (person == NULL) {
        return;
    }

    Observer **current = &person->observers;

    while (*current != NULL) {
        Observer *observer = *current;

        if (observer->context == context &&
            observer->callback == callback) {
            *current = observer->next;
            free(observer);
            return;
        }

        current = &observer->next;
    }
}

void person_set_name(Person *person, const char *name) {
    if (person == NULL || name == NULL ||
        strcmp(person->name, name) == 0) {
        return;
    }

    char *new_name = malloc(strlen(name) + 1);
    if (new_name == NULL) {
        return;
    }

    strcpy(new_name, name);

    char *old_name = person->name;
    person->name = new_name;

    notify_observers(person, "name", old_name, new_name);

    free(old_name);
}

void person_set_age(Person *person, int age) {
    if (person == NULL || person->age == age) {
        return;
    }

    int old_age = person->age;
    person->age = age;

    notify_observers(person, "age", &old_age, &person->age);
}

void on_person_property_changed(
    void *context,
    const PropertyChangeEvent *event
) {
    (void)context;

    if (strcmp(event->property_name, "name") == 0) {
        printf("Name changed: %s -> %s\n",
               (const char *)event->old_value,
               (const char *)event->new_value);
    } else if (strcmp(event->property_name, "age") == 0) {
        printf("Age changed: %d -> %d\n",
               *(const int *)event->old_value,
               *(const int *)event->new_value);
    }
}

int main(void) {
    Person person;
    person_init(&person, "Alice", 30);

    person_add_observer(
        &person,
        NULL,
        on_person_property_changed
    );

    person_set_name(&person, "Bob");
    person_set_age(&person, 31);

    person_remove_observer(
        &person,
        NULL,
        on_person_property_changed
    );

    person_destroy(&person);
    return 0;
}
