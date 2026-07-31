#import <Foundation/Foundation.h>

@interface BatchWorker : NSObject
- (instancetype)initWithBatchSize:(NSUInteger)batchSize
                       timeInterval:(NSTimeInterval)timeInterval
                            handler:(void (^)(NSArray *items))handler;
- (void)addItem:(id)item;
- (void)stop;
@end

@interface BatchWorker ()
@property (nonatomic) NSUInteger batchSize;
@property (nonatomic) NSTimeInterval timeInterval;
@property (nonatomic, copy) void (^handler)(NSArray *);
@property (nonatomic, strong) NSMutableArray *items;
@property (nonatomic, strong) NSCondition *condition;
@property (nonatomic) BOOL stopping;
@property (nonatomic, strong) NSThread *thread;
@end

@implementation BatchWorker

- (instancetype)initWithBatchSize:(NSUInteger)batchSize
                       timeInterval:(NSTimeInterval)timeInterval
                            handler:(void (^)(NSArray *))handler {
    NSParameterAssert(batchSize > 0);
    NSParameterAssert(timeInterval > 0);
    NSParameterAssert(handler != nil);

    self = [super init];
    if (self) {
        _batchSize = batchSize;
        _timeInterval = timeInterval;
        _handler = [handler copy];
        _items = [NSMutableArray arrayWithCapacity:batchSize];
        _condition = [[NSCondition alloc] init];
        _thread = [[NSThread alloc] initWithTarget:self
                                          selector:@selector(workerLoop)
                                            object:nil];
        [_thread start];
    }
    return self;
}

- (void)addItem:(id)item {
    if (!item) return;

    [self.condition lock];
    if (!self.stopping) {
        [self.items addObject:item];
        if (self.items.count >= self.batchSize) {
            [self.condition signal];
        }
    }
    [self.condition unlock];
}

- (void)stop {
    [self.condition lock];
    if (!self.stopping) {
        self.stopping = YES;
        [self.condition signal];
    }
    [self.condition unlock];

    while (!self.thread.isFinished) {
        [NSThread sleepForTimeInterval:0.001];
    }
}

- (void)workerLoop {
    @autoreleasepool {
        while (YES) {
            NSArray *batch = nil;

            [self.condition lock];

            while (!self.stopping && self.items.count == 0) {
                [self.condition wait];
            }

            if (self.stopping && self.items.count == 0) {
                [self.condition unlock];
                break;
            }

            if (self.items.count < self.batchSize && !self.stopping) {
                NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:self.timeInterval];
                [self.condition waitUntilDate:deadline];
            }

            if (self.items.count > 0) {
                batch = [self.items copy];
                [self.items removeAllObjects];
            }

            BOOL shouldExit = self.stopping && self.items.count == 0;
            [self.condition unlock];

            if (batch.count > 0) {
                @autoreleasepool {
                    self.handler(batch);
                }
            }

            if (shouldExit) {
                break;
            }
        }
    }
}

- (void)dealloc {
    [self stop];
}

@end】【：】【“】【
