#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VSVector3D : NSObject <NSCopying>

@property (nonatomic, readonly) double x;
@property (nonatomic, readonly) double y;
@property (nonatomic, readonly) double z;

- (instancetype)initWithX:(double)x
                         y:(double)y
                         z:(double)z;

- (double)dotProductWithVector:(VSVector3D *)vector;
- (VSVector3D *)crossProductWithVector:(VSVector3D *)vector;

@end

@implementation VSVector3D

- (instancetype)initWithX:(double)x
                         y:(double)y
                         z:(double)z {
    self = [super init];
    if (self) {
        _x = x;
        _y = y;
        _z = z;
    }
    return self;
}

- (double)dotProductWithVector:(VSVector3D *)vector {
    NSParameterAssert(vector != nil);
    return (self.x * vector.x) +
           (self.y * vector.y) +
           (self.z * vector.z);
}

- (VSVector3D *)crossProductWithVector:(VSVector3D *)vector {
    NSParameterAssert(vector != nil);

    return [[VSVector3D alloc] initWithX:(self.y * vector.z) - (self.z * vector.y)
                                       y:(self.z * vector.x) - (self.x * vector.z)
                                       z:(self.x * vector.y) - (self.y * vector.x)];
}

- (id)copyWithZone:(NSZone *)zone {
    return self;
}

- (BOOL)isEqual:(id)object {
    if (self == object) {
        return YES;
    }

    if (![object isKindOfClass:[VSVector3D class]]) {
        return NO;
    }

    VSVector3D *vector = (VSVector3D *)object;
    return self.x == vector.x &&
           self.y == vector.y &&
           self.z == vector.z;
}

- (NSUInteger)hash {
    return @(self.x).hash ^ @(self.y).hash ^ @(self.z).hash;
}

@end

NS_ASSUME_NONNULL_END
