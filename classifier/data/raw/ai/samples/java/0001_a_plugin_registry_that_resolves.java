import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;

/**
 * A registry that maps names to plugin handlers and resolves them by name.
 *
 * @param <H> the handler type
 */
public final class PluginRegistry<H> {

    private final Map<String, H> handlers = new LinkedHashMap<>();

    /**
     * Registers a handler under the specified name.
     *
     * @param name the unique handler name
     * @param handler the handler to register
     * @throws NullPointerException if {@code name} or {@code handler} is null
     * @throws IllegalArgumentException if a handler is already registered under
     *         the specified name
     */
    public void register(String name, H handler) {
        Objects.requireNonNull(name, "name");
        Objects.requireNonNull(handler, "handler");

        if (handlers.containsKey(name)) {
            throw new IllegalArgumentException(
                    "A handler is already registered under name: " + name);
        }

        handlers.put(name, handler);
    }

    /**
     * Replaces or registers a handler under the specified name.
     *
     * @param name the handler name
     * @param handler the handler to register
     * @return the previously registered handler, or an empty optional if none existed
     * @throws NullPointerException if {@code name} or {@code handler} is null
     */
    public Optional<H> registerOrReplace(String name, H handler) {
        Objects.requireNonNull(name, "name");
        Objects.requireNonNull(handler, "handler");
        return Optional.ofNullable(handlers.put(name, handler));
    }

    /**
     * Resolves a handler by name.
     *
     * @param name the handler name
     * @return the matching handler
     * @throws NullPointerException if {@code name} is null
     * @throws PluginNotFoundException if no handler is registered under the name
     */
    public H resolve(String name) {
        Objects.requireNonNull(name, "name");

        H handler = handlers.get(name);
        if (handler == null) {
            throw new PluginNotFoundException(name);
        }

        return handler;
    }

    /**
     * Resolves a handler by name without throwing if it is absent.
     *
     * @param name the handler name
     * @return an optional containing the matching handler, or empty if absent
     * @throws NullPointerException if {@code name} is null
     */
    public Optional<H> find(String name) {
        Objects.requireNonNull(name, "name");
        return Optional.ofNullable(handlers.get(name));
    }

    /**
     * Removes a handler by name.
     *
     * @param name the handler name
     * @return the removed handler, or an empty optional if absent
     * @throws NullPointerException if {@code name} is null
     */
    public Optional<H> unregister(String name) {
        Objects.requireNonNull(name, "name");
        return Optional.ofNullable(handlers.remove(name));
    }

    /**
     * Determines whether a handler is registered under the specified name.
     *
     * @param name the handler name
     * @return {@code true} if a handler is registered under the name
     * @throws NullPointerException if {@code name} is null
     */
    public boolean contains(String name) {
        Objects.requireNonNull(name, "name");
        return handlers.containsKey(name);
    }

    /**
     * Returns the number of registered handlers.
     *
     * @return the number of registered handlers
     */
    public int size() {
        return handlers.size();
    }

    /**
     * Returns an immutable view of the registered handlers.
     *
     * @return an immutable map of handler names to handlers
     */
    public Map<String, H> handlers() {
        return Collections.unmodifiableMap(handlers);
    }

    /**
     * Removes all registered handlers.
     */
    public void clear() {
        handlers.clear();
    }

    /**
     * Exception thrown when a requested plugin handler cannot be found.
     */
    public static final class PluginNotFoundException extends RuntimeException {

        /**
         * Creates an exception for a missing plugin name.
         *
         * @param name the missing plugin name
         */
        public PluginNotFoundException(String name) {
            super("No handler registered under name: " + name);
        }
    }
}
