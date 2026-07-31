#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct PropertySubject PropertySubject;

/**
 * @brief Callback invoked when a property changes.
 *
 * @param subject The subject whose property changed.
 * @param property_name The name of the changed property.
 * @param old_value The previous property value, or NULL if unset.
 * @param new_value The new property value, or NULL if unset.
 * @param user_data User-provided callback data.
 */
typedef void (*PropertyObserver)(PropertySubject *subject,
                                 const char *property_name,
                                 const char *old_value,
                                 const char *new_value,
                                 void *user_data);

/**
 * @brief Creates a new property subject.
 *
 * @return A newly allocated subject, or NULL on allocation failure.
 */
PropertySubject *property_subject_create(void);

/**
 * @brief Destroys a property subject and releases all associated resources.
 *
 * @param subject The subject to destroy.
 */
void property_subject_destroy(PropertySubject *subject);

/**
 * @brief Registers an observer for property change notifications.
 *
 * @param subject The subject to observe.
 * @param observer The callback to invoke when a property changes.
 * @param user_data User-provided data passed to the callback.
 *
 * @return 0 on success, or -1 on failure.
 */
int property_subject_add_observer(PropertySubject *subject,
                                  PropertyObserver observer,
                                  void *user_data);

/**
 * @brief Removes a previously registered observer.
 *
 * @param subject The subject being observed.
 * @param observer The observer callback to remove.
 * @param user_data The user data associated with the observer.
 *
 * @return 0 if removed, or -1 if not found or arguments are invalid.
 */
int property_subject_remove_observer(PropertySubject *subject,
                                     PropertyObserver observer,
                                     void *user_data);

/**
 * @brief Sets a property value and notifies observers if it changed.
 *
 * @param subject The subject whose property should be changed.
 * @param property_name The property name.
 * @param value The new value, or NULL to unset the property.
 *
 * @return 0 on success, or -1 on failure.
 */
int property_subject_set(PropertySubject *subject,
                         const char *property_name,
                         const char *value);

/**
 * @brief Gets the current value of a property.
 *
 * @param subject The subject containing the property.
 * @param property_name The property name.
 *
 * @return The property value, or NULL if it does not exist.
 *
 * @note The returned pointer is owned by the subject and becomes invalid
 *       when the property changes or the subject is destroyed.
 */
const char *property_subject_get(const PropertySubject *subject,
                                 const char *property_name);

typedef struct {
    char *name;
    char *value;
} PropertyEntry;

typedef struct {
    PropertyObserver callback;
    void *user_data;
} ObserverEntry;

struct PropertySubject {
    PropertyEntry *properties;
    size_t property_count;
    size_t property_capacity;

    ObserverEntry *observers;
    size_t observer_count;
    size_t observer_capacity;
};

static char *duplicate_string(const char *value)
{
    size_t length;
    char *copy;

    if (value == NULL) {
        return NULL;
    }

    length = strlen(value) + 1;
    copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, value, length);
    }

    return copy;
}

static int strings_equal(const char *left, const char *right)
{
    if (left == NULL || right == NULL) {
        return left == right;
    }

    return strcmp(left, right) == 0;
}

PropertySubject *property_subject_create(void)
{
    return calloc(1, sizeof(PropertySubject));
}

void property_subject_destroy(PropertySubject *subject)
{
    size_t i;

    if (subject == NULL) {
        return;
    }

    for (i = 0; i < subject->property_count; ++i) {
        free(subject->properties[i].name);
        free(subject->properties[i].value);
    }

    free(subject->properties);
    free(subject->observers);
    free(subject);
}

int property_subject_add_observer(PropertySubject *subject,
                                  PropertyObserver observer,
                                  void *user_data)
{
    ObserverEntry *new_observers;
    size_t new_capacity;

    if (subject == NULL || observer == NULL) {
        return -1;
    }

    if (subject->observer_count == subject->observer_capacity) {
        new_capacity = subject->observer_capacity == 0
            ? 4
            : subject->observer_capacity * 2;

        new_observers = realloc(subject->observers,
                                new_capacity * sizeof(ObserverEntry));
        if (new_observers == NULL) {
            return -1;
        }

        subject->observers = new_observers;
        subject->observer_capacity = new_capacity;
    }

    subject->observers[subject->observer_count].callback = observer;
    subject->observers[subject->observer_count].user_data = user_data;
    subject->observer_count++;

    return 0;
}

int property_subject_remove_observer(PropertySubject *subject,
                                     PropertyObserver observer,
                                     void *user_data)
{
    size_t i;

    if (subject == NULL || observer == NULL) {
        return -1;
    }

    for (i = 0; i < subject->observer_count; ++i) {
        if (subject->observers[i].callback == observer &&
            subject->observers[i].user_data == user_data) {
            subject->observers[i] =
                subject->observers[subject->observer_count - 1];
            subject->observer_count--;
            return 0;
        }
    }

    return -1;
}

int property_subject_set(PropertySubject *subject,
                         const char *property_name,
                         const char *value)
{
    size_t i;
    char *new_value;
    char *old_value;
    PropertyEntry *new_properties;
    size_t new_capacity;

    if (subject == NULL || property_name == NULL || property_name[0] == '\0') {
        return -1;
    }

    for (i = 0; i < subject->property_count; ++i) {
        if (strcmp(subject->properties[i].name, property_name) == 0) {
            if (strings_equal(subject->properties[i].value, value)) {
                return 0;
            }

            new_value = duplicate_string(value);
            if (value != NULL && new_value == NULL) {
                return -1;
            }

            old_value = subject->properties[i].value;
            subject->properties[i].value = new_value;

            for (i = 0; i < subject->observer_count; ++i) {
                subject->observers[i].callback(subject,
                                               property_name,
                                               old_value,
                                               new_value,
                                               subject->observers[i].user_data);
            }

            free(old_value);
            return 0;
        }
    }

    if (value == NULL) {
        return 0;
    }

    if (subject->property_count == subject->property_capacity) {
        new_capacity = subject->property_capacity == 0
            ? 4
            : subject->property_capacity * 2;

        new_properties = realloc(subject->properties,
                                  new_capacity * sizeof(PropertyEntry));
        if (new_properties == NULL) {
            return -1;
        }

        subject->properties = new_properties;
        subject->property_capacity = new_capacity;
    }

    subject->properties[subject->property_count].name =
        duplicate_string(property_name);
    subject->properties[subject->property_count].value =
        duplicate_string(value);

    if (subject->properties[subject->property_count].name == NULL ||
        subject->properties[subject->property_count].value == NULL) {
        free(subject->properties[subject->property_count].name);
        free(subject->properties[subject->property_count].value);
        return -1;
    }

    subject->property_count++;

    for (i = 0; i < subject->observer_count; ++i) {
        subject->observers[i].callback(subject,
                                       property_name,
                                       NULL,
                                       value,
                                       subject->observers[i].user_data);
    }

    return 0;
}

const char *property_subject_get(const PropertySubject *subject,
                                 const char *property_name)
{
    size_t i;

    if (subject == NULL || property_name == NULL) {
        return NULL;
    }

    for (i = 0; i < subject->property_count; ++i) {
        if (strcmp(subject->properties[i].name, property_name) == 0) {
            return subject->properties[i].value;
        }
    }

    return NULL;
}
