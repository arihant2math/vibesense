import java.util.Arrays;

public final class Dijkstra {
    private final int[] head;
    private int[] to;
    private int[] weight;
    private int[] next;
    private int edgeCount;

    public Dijkstra(int vertices, int edges) {
        head = new int[vertices];
        Arrays.fill(head, -1);
        to = new int[edges];
        weight = new int[edges];
        next = new int[edges];
    }

    public void addEdge(int from, int destination, int cost) {
        if (cost < 0) {
            throw new IllegalArgumentException("Negative edge weight");
        }
        if (edgeCount == to.length) {
            int newSize = Math.max(1, to.length * 2);
            to = Arrays.copyOf(to, newSize);
            weight = Arrays.copyOf(weight, newSize);
            next = Arrays.copyOf(next, newSize);
        }

        to[edgeCount] = destination;
        weight[edgeCount] = cost;
        next[edgeCount] = head[from];
        head[from] = edgeCount++;
    }

    public long[] shortestPaths(int source) {
        long[] distance = new long[head.length];
        Arrays.fill(distance, Long.MAX_VALUE);

        MinHeap heap = new MinHeap();
        distance[source] = 0;
        heap.add(source, 0);

        while (!heap.isEmpty()) {
            int vertex = heap.pollVertex();
            long currentDistance = heap.pollDistance();

            if (currentDistance != distance[vertex]) {
                continue;
            }

            for (int edge = head[vertex]; edge != -1; edge = next[edge]) {
                int neighbor = to[edge];
                long candidate = currentDistance + weight[edge];

                if (candidate < distance[neighbor]) {
                    distance[neighbor] = candidate;
                    heap.add(neighbor, candidate);
                }
            }
        }

        return distance;
    }

    private static final class MinHeap {
        private int[] vertices = new int[16];
        private long[] distances = new long[16];
        private int size;
        private int lastVertex;
        private long lastDistance;

        void add(int vertex, long distance) {
            if (size == vertices.length) {
                int newSize = vertices.length * 2;
                vertices = Arrays.copyOf(vertices, newSize);
                distances = Arrays.copyOf(distances, newSize);
            }

            int index = size++;
            while (index > 0) {
                int parent = (index - 1) >>> 1;
                if (distances[parent] <= distance) {
                    break;
                }
                vertices[index] = vertices[parent];
                distances[index] = distances[parent];
                index = parent;
            }

            vertices[index] = vertex;
            distances[index] = distance;
        }

        boolean isEmpty() {
            return size == 0;
        }

        int pollVertex() {
            removeMin();
            return lastVertex;
        }

        long pollDistance() {
            return lastDistance;
        }

        private void removeMin() {
            lastVertex = vertices[0];
            lastDistance = distances[0];

            int newSize = --size;
            if (newSize == 0) {
                return;
            }

            int vertex = vertices[newSize];
            long distance = distances[newSize];
            int index = 0;

            while (true) {
                int left = index * 2 + 1;
                if (left >= newSize) {
                    break;
                }

                int right = left + 1;
                int child = left;

                if (right < newSize && distances[right] < distances[left]) {
                    child = right;
                }

                if (distances[child] >= distance) {
                    break;
                }

                vertices[index] = vertices[child];
                distances[index] = distances[child];
                index = child;
            }

            vertices[index] = vertex;
            distances[index] = distance;
        }
    }
}
