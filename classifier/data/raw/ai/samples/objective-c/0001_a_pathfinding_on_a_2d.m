#import <Foundation/Foundation.h>

@interface Node : NSObject
@property(nonatomic) NSInteger x, y;
@property(nonatomic) NSInteger g, h;
@property(nonatomic, weak) Node *parent;
- (instancetype)initWithX:(NSInteger)x y:(NSInteger)y;
- (NSInteger)f;
@end

@implementation Node

- (instancetype)initWithX:(NSInteger)x y:(NSInteger)y {
    if (self = [super init]) {
        _x = x;
        _y = y;
        _g = NSIntegerMax;
        _h = 0;
    }
    return self;
}

- (NSInteger)f {
    if (_g == NSIntegerMax) return NSIntegerMax;
    return _g + _h;
}

@end

static NSInteger Heuristic(NSInteger x1, NSInteger y1, NSInteger x2, NSInteger y2) {
    return llabs(x1 - x2) + llabs(y1 - y2);
}

static NSArray<Node *> *AStar(NSArray<NSArray<NSNumber *> *> *grid,
                              NSInteger startX, NSInteger startY,
                              NSInteger goalX, NSInteger goalY) {
    NSInteger height = grid.count;
    NSInteger width = grid.firstObject.count;
    
    if (startX < 0 || startY < 0 || goalX < 0 || goalY < 0 ||
        startX >= width || goalX >= width ||
        startY >= height || goalY >= height ||
        grid[startY][startX].boolValue ||
        grid[goalY][goalX].boolValue) {
        return @[];
    }
    
    NSMutableDictionary<NSString *, Node *> *nodes = [NSMutableDictionary dictionary];
    NSMutableArray<Node *> *open = [NSMutableArray array];
    NSMutableSet<NSString *> *closed = [NSMutableSet set];
    
    NSString *(^Key)(NSInteger, NSInteger) = ^NSString *(NSInteger x, NSInteger y) {
        return [NSString stringWithFormat:@"%ld,%ld", (long)x, (long)y];
    };
    
    Node *start = [[Node alloc] initWithX:startX y:startY];
    start.g = 0;
    start.h = Heuristic(startX, startY, goalX, goalY);
    nodes[Key(startX, startY)] = start;
    [open addObject:start];
    
    NSInteger directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    
    while (open.count > 0) {
        NSUInteger bestIndex = 0;
        for (NSUInteger i = 1; i < open.count; i++) {
            if (open[i].f < open[bestIndex].f ||
                (open[i].f == open[bestIndex].f && open[i].h < open[bestIndex].h)) {
                bestIndex = i;
            }
        }
        
        Node *current = open[bestIndex];
        [open removeObjectAtIndex:bestIndex];
        NSString *currentKey = Key(current.x, current.y);
        
        if (current.x == goalX && current.y == goalY) {
            NSMutableArray<Node *> *path = [NSMutableArray array];
            for (Node *node = current; node != nil; node = node.parent) {
                [path insertObject:node atIndex:0];
            }
            return path;
        }
        
        [closed addObject:currentKey];
        
        for (NSInteger i = 0; i < 4; i++) {
            NSInteger nx = current.x + directions[i][0];
            NSInteger ny = current.y + directions[i][1];
            
            if (nx < 0 || ny < 0 || nx >= width || ny >= height ||
                grid[ny][nx].boolValue) {
                continue;
            }
            
            NSString *neighborKey = Key(nx, ny);
            if ([closed containsObject:neighborKey]) continue;
            
            Node *neighbor = nodes[neighborKey];
            if (!neighbor) {
                neighbor = [[Node alloc] initWithX:nx y:ny];
                neighbor.h = Heuristic(nx, ny, goalX, goalY);
                nodes[neighborKey] = neighbor;
            }
            
            NSInteger tentativeG = current.g + 1;
            if (tentativeG < neighbor.g) {
                neighbor.g = tentativeG;
                neighbor.parent = current;
                if (![open containsObject:neighbor]) {
                    [open addObject:neighbor];
                }
            }
        }
    }
    
    return @[];
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSArray *grid = @[
            @[@0, @0, @0, @0, @0, @0, @0, @0],
            @[@0, @1, @1, @1, @0, @0, @0, @0],
            @[@0, @0, @0, @1, @0, @0, @0, @0],
            @[@0, @0, @0, @1, @0, @1, @1, @0],
            @[@0, @0, @0, @0, @0, @0, @0, @0],
            @[@0, @1, @1, @1, @1, @1, @0, @0],
            @[@0, @0, @0, @0, @0, @0, @0, @0]
        ];
        
        NSArray<Node *> *path = AStar(grid, 0, 0, 7, 6);
        
        if (path.count == 0) {
            NSLog(@"No path found.");
        } else {
            NSLog(@"Path:");
            for (Node *node in path) {
                NSLog(@"(%ld, %ld)", (long)node.x, (long)node.y);
            }
        }
    }
    return 0;
}
