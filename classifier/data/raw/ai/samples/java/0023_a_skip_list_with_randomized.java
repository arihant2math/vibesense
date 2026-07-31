import java.util.Arrays;
import java.util.Random;

public class SkipList<T extends Comparable<T>> {
    private static final int MAX_LEVEL = 32;
    private static final double LEVEL_PROBABILITY = 0.5;

    private final Random random;
    private final Node<T> head;
    private int level;
    private int size;

    public SkipList() {
        this(new Random());
    }

    public SkipList(Random random) {
        this.random = random;
        this.head = new Node<>(null, MAX_LEVEL);
        this.level = 1;
        this.size = 0;
    }

    public void add(T value) {
        if (value == null) {
            throw new IllegalArgumentException("Value cannot be null");
        }

        @SuppressWarnings("unchecked")
        Node<T>[] update = (Node<T>[]) new Node[MAX_LEVEL];

        Node<T> current = head;

        for (int i = level - 1; i >= 0; i--) {
            while (current.next[i] != null &&
                   current.next[i].value.compareTo(value) < 0) {
                current = current.next[i];
            }
            update[i] = current;
        }

        Node<T> existing = current.next[0];
        if (existing != null && existing.value.compareTo(value) == 0) {
            return;
        }

        int newLevel = randomLevel();

        if (newLevel > level) {
            for (int i = level; i < newLevel; i++) {
                update[i] = head;
            }
            level = newLevel;
        }

        Node<T> newNode = new Node<>(value, newLevel);

        for (int i = 0; i < newLevel; i++) {
            newNode.next[i] = update[i].next[i];
            update[i].next[i] = newNode;
        }

        size++;
    }

    public boolean contains(T value) {
        return find(value) != null;
    }

    public boolean remove(T value) {
        if (value == null) {
            return false;
        }

        @SuppressWarnings("unchecked")
        Node<T>[] update = (Node<T>[]) new Node[MAX_LEVEL];

        Node<T> current = head;

        for (int i = level - 1; i >= 0; i--) {
            while (current.next[i] != null &&
                   current.next[i].value.compareTo(value) < 0) {
                current = current.next[i];
            }
            update[i] = current;
        }

        Node<T> target = current.next[0];

        if (target == null || target.value.compareTo(value) != 0) {
            return false;
        }

        for (int i = 0; i < level; i++) {
            if (update[i].next[i] != target) {
                break;
            }
            update[i].next[i] = target.next[i];
        }

        while (level > 1 && head.next[level - 1] == null) {
            level--;
        }

        size--;
        return true;
    }

    public T find(T value) {
        if (value == null) {
            return null;
        }

        Node<T> current = head;

        for (int i = level - 1; i >= 0; i--) {
            while (current.next[i] != null &&
                   current.next[i].value.compareTo(value) < 0) {
                current = current.next[i];
            }
        }

        current = current.next[0];

        if (current != null && current.value.compareTo(value) == 0) {
            return current.value;
        }

        return null;
    }

    public int size() {
        return size;
    }

    public boolean isEmpty() {
        return size == 0;
    }

    public void clear() {
        Arrays.fill(head.next, null);
        level = 1;
        size = 0;
    }

    private int randomLevel() {
        int result = 1;

        while (result < MAX_LEVEL &&
               random.nextDouble() < LEVEL_PROBABILITY) {
            result++;
        }

        return result;
    }

    private static class Node<T> {
        private final T value;
        private final Node<T>[] next;

        @SuppressWarnings("unchecked")
        private Node(T value, int level) {
            this.value = value;
            this.next = (Node<T>[]) new Node[level];
        }
    }
}
