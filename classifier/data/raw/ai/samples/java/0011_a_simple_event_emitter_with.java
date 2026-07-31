import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.Consumer;

public class EventEmitter<T> {
    private final List<Consumer<T>> listeners = new CopyOnWriteArrayList<>();

    /**
     * Creates an empty event emitter.
     */
    public EventEmitter() {
    }

    /**
     * Subscribes a listener to receive emitted events.
     *
     * @param listener the listener to subscribe
     * @return a subscription that can be used to unsubscribe the listener
     */
    public Subscription subscribe(Consumer<T> listener) {
        if (listener == null) {
            throw new IllegalArgumentException("Listener cannot be null");
        }

        listeners.add(listener);
        return () -> listeners.remove(listener);
    }

    /**
     * Emits an event to all currently subscribed listeners.
     *
     * @param event the event to emit
     */
    public void emit(T event) {
        for (Consumer<T> listener : listeners) {
            listener.accept(event);
        }
    }

    /**
     * Removes all subscribed listeners.
     */
    public void clear() {
        listeners.clear();
    }

    /**
     * Represents a subscription that can be canceled.
     */
    @FunctionalInterface
    public interface Subscription {
        /**
         * Unsubscribes the associated listener.
         */
        void unsubscribe();
    }
}
