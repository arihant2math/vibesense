import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Deque;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Provides topological sorting for directed graphs with cycle detection.
 */
public final class TopologicalSort {
    private TopologicalSort() {
    }

    /**
     * Returns a topological ordering of the vertices in a directed graph.
     *
     * @param graph the directed graph represented as an adjacency list
     * @param <T> the vertex type
     * @return a topological ordering of the graph's vertices
     * @throws IllegalArgumentException if the graph contains a cycle
     */
    public static <T> List<T> sort(Map<T, ? extends Iterable<T>> graph) {
        Map<T, Integer> inDegree = new HashMap<>();
        Map<T, List<T>> adjacency = new HashMap<>();

        for (Map.Entry<T, ? extends Iterable<T>> entry : graph.entrySet()) {
            T vertex = entry.getKey();
            inDegree.putIfAbsent(vertex, 0);
            adjacency.putIfAbsent(vertex, new ArrayList<>());

            for (T neighbor : entry.getValue()) {
                adjacency.get(vertex).add(neighbor);
                inDegree.put(neighbor, inDegree.getOrDefault(neighbor, 0) + 1);
            }
        }

        Deque<T> queue = new ArrayDeque<>();
        for (Map.Entry<T, Integer> entry : inDegree.entrySet()) {
            if (entry.getValue() == 0) {
                queue.add(entry.getKey());
            }
        }

        List<T> ordering = new ArrayList<>();

        while (!queue.isEmpty()) {
            T vertex = queue.remove();
            ordering.add(vertex);

            for (T neighbor : adjacency.getOrDefault(vertex, Collections.emptyList())) {
                int updatedDegree = inDegree.get(neighbor) - 1;
                inDegree.put(neighbor, updatedDegree);

                if (updatedDegree == 0) {
                    queue.add(neighbor);
                }
            }
        }

        if (ordering.size() != inDegree.size()) {
            throw new IllegalArgumentException("Graph contains a cycle");
        }

        return ordering;
    }

    /**
     * Determines whether a directed graph contains a cycle.
     *
     * @param graph the directed graph represented as an adjacency list
     * @param <T> the vertex type
     * @return {@code true} if the graph contains a cycle; otherwise {@code false}
     */
    public static <T> boolean hasCycle(Map<T, ? extends Iterable<T>> graph) {
        try {
            sort(graph);
            return false;
        } catch (IllegalArgumentException exception) {
            return true;
        }
    }
}
