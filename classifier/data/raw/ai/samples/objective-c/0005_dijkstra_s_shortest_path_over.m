#import <Foundation/Foundation.h>

@interface Edge : NSObject
@property(nonatomic) NSInteger destination;
@property(nonatomic) NSInteger weight;
+ (instancetype)edgeTo:(NSInteger)destination weight:(NSInteger)weight;
@end

@implementation Edge
+ (instancetype)edgeTo:(NSInteger)destination weight:(NSInteger)weight {
    Edge *edge = [Edge new];
    edge.destination = destination;
    edge.weight = weight;
    return edge;
}
@end

@interface Graph : NSObject
@property(nonatomic) NSInteger vertexCount;
@property(nonatomic, strong) NSMutableArray<NSMutableArray<Edge *> *> *adjacencyList;
- (instancetype)initWithVertexCount:(NSInteger)vertexCount;
- (void)addEdgeFrom:(NSInteger)source to:(NSInteger)destination weight:(NSInteger)weight;
- (NSArray<NSNumber *> *)shortestPathsFrom:(NSInteger)source;
@end

@implementation Graph

- (instancetype)initWithVertexCount:(NSInteger)vertexCount {
    self = [super init];
    if (self) {
        _vertexCount = vertexCount;
        _adjacencyList = [NSMutableArray arrayWithCapacity:vertexCount];
        for (NSInteger i = 0; i < vertexCount; i++) {
            [_adjacencyList addObject:[NSMutableArray array]];
        }
    }
    return self;
}

- (void)addEdgeFrom:(NSInteger)source to:(NSInteger)destination weight:(NSInteger)weight {
    if (source >= 0 && source < _vertexCount &&
        destination >= 0 && destination < _vertexCount &&
        weight >= 0) {
        [_adjacencyList[source] addObject:[Edge edgeTo:destination weight:weight]];
    }
}

- (NSArray<NSNumber *> *)shortestPathsFrom:(NSInteger)source {
    const NSInteger infinity = NSIntegerMax;
    NSMutableArray<NSNumber *> *distances = [NSMutableArray arrayWithCapacity:_vertexCount];
    NSMutableArray<NSNumber *> *visited = [NSMutableArray arrayWithCapacity:_vertexCount];

    for (NSInteger i = 0; i < _vertexCount; i++) {
        [distances addObject:@(infinity)];
        [visited addObject:@NO];
    }

    if (source < 0 || source >= _vertexCount) {
        return distances;
    }

    distances[source] = @0;

    for (NSInteger iteration = 0; iteration < _vertexCount; iteration++) {
        NSInteger current = -1;
        NSInteger smallestDistance = infinity;

        for (NSInteger vertex = 0; vertex < _vertexCount; vertex++) {
            if (!visited[vertex].boolValue &&
                distances[vertex].integerValue < smallestDistance) {
                smallestDistance = distances[vertex].integerValue;
                current = vertex;
            }
        }

        if (current == -1) {
            break;
        }

        visited[current] = @YES;

        for (Edge *edge in _adjacencyList[current]) {
            NSInteger newDistance = smallestDistance + edge.weight;
            if (newDistance < distances[edge.destination].integerValue) {
                distances[edge.destination] = @(newDistance);
            }
        }
    }

    return distances;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        Graph *graph = [[Graph alloc] initWithVertexCount:6];

        [graph addEdgeFrom:0 to:1 weight:4];
        [graph addEdgeFrom:0 to:2 weight:2];
        [graph addEdgeFrom:1 to:2 weight:1];
        [graph addEdgeFrom:1 to:3 weight:5];
        [graph addEdgeFrom:2 to:3 weight:8];
        [graph addEdgeFrom:2 to:4 weight:10];
        [graph addEdgeFrom:3 to:4 weight:2];
        [graph addEdgeFrom:3 to:5 weight:6];
        [graph addEdgeFrom:4 to:5 weight:3];

        NSArray<NSNumber *> *distances = [graph shortestPathsFrom:0];

        for (NSInteger vertex = 0; vertex < distances.count; vertex++) {
            NSInteger distance = distances[vertex].integerValue;
            if (distance == NSIntegerMax) {
                NSLog(@"Vertex %ld: unreachable", (long)vertex);
            } else {
                NSLog(@"Vertex %ld: %ld", (long)vertex, (long)distance);
            }
        }
    }

    return 0;
}
