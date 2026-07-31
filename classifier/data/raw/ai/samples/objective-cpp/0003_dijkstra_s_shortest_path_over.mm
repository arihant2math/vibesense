#import <Foundation/Foundation.h>
#include <vector>
#include <queue>
#include <limits>
#include <utility>

struct Edge {
    int destination;
    int weight;
};

std::vector<long long> Dijkstra(
    const std::vector<std::vector<Edge>>& graph,
    int source
) {
    const long long infinity = std::numeric_limits<long long>::max();
    const int vertexCount = static_cast<int>(graph.size());

    std::vector<long long> distances(vertexCount, infinity);
    std::priority_queue<
        std::pair<long long, int>,
        std::vector<std::pair<long long, int>>,
        std::greater<std::pair<long long, int>>
    > queue;

    if (source < 0 || source >= vertexCount) {
        return distances;
    }

    distances[source] = 0;
    queue.emplace(0, source);

    while (!queue.empty()) {
        const auto [currentDistance, vertex] = queue.top();
        queue.pop();

        if (currentDistance != distances[vertex]) {
            continue;
        }

        for (const Edge& edge : graph[vertex]) {
            if (edge.weight < 0) {
                continue;
            }

            if (currentDistance > infinity - edge.weight) {
                continue;
            }

            const long long candidateDistance =
                currentDistance + edge.weight;

            if (candidateDistance < distances[edge.destination]) {
                distances[edge.destination] = candidateDistance;
                queue.emplace(candidateDistance, edge.destination);
            }
        }
    }

    return distances;
}
