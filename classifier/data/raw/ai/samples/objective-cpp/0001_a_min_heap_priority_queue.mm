#import <Foundation/Foundation.h>

@interface MinHeap : NSObject

- (void)insert:(NSInteger)value;
- (NSInteger)extractMin;
- (BOOL)isEmpty;
- (NSUInteger)count;

@end

@implementation MinHeap {
    NSMutableArray<NSNumber *> *_heap;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _heap = [NSMutableArray array];
    }
    return self;
}

- (void)insert:(NSInteger)value {
    [_heap addObject:@(value)];
    NSUInteger index = _heap.count - 1;

    while (index > 0) {
        NSUInteger parent = (index - 1) / 2;
        if (_heap[parent].integerValue <= _heap[index].integerValue) {
            break;
        }

        [_heap exchangeObjectAtIndex:index withObjectAtIndex:parent];
        index = parent;
    }
}

- (NSInteger)extractMin {
    if (_heap.count == 0) {
        [NSException raise:NSRangeException format:@"Cannot extract from an empty heap"];
    }

    NSInteger minimum = _heap[0].integerValue;
    NSNumber *last = _heap.lastObject;
    [_heap removeLastObject];

    if (_heap.count > 0) {
        _heap[0] = last;
        NSUInteger index = 0;

        while (YES) {
            NSUInteger left = index * 2 + 1;
            NSUInteger right = index * 2 + 2;
            NSUInteger smallest = index;

            if (left < _heap.count &&
                _heap[left].integerValue < _heap[smallest].integerValue) {
                smallest = left;
            }

            if (right < _heap.count &&
                _heap[right].integerValue < _heap[smallest].integerValue) {
                smallest = right;
            }

            if (smallest == index) {
                break;
            }

            [_heap exchangeObjectAtIndex:index withObjectAtIndex:smallest];
            index = smallest;
        }
    }

    return minimum;
}

- (BOOL)isEmpty {
    return _heap.count == 0;
}

- (NSUInteger)count {
    return _heap.count;
}

@end

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        MinHeap *priorityQueue = [[MinHeap alloc] init];

        for (NSInteger value in @[ @42, @7, @19, @3, @25, @1 ]) {
            [priorityQueue insert:value];
        }

        while (![priorityQueue isEmpty]) {
            NSLog(@"%ld", (long)[priorityQueue extractMin]);
        }
    }

    return 0;
}
