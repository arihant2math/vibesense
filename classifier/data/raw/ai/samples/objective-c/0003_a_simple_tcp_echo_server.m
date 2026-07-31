#import <Foundation/Foundation.h>
#import <sys/socket.h>
#import <netinet/in.h>
#import <arpa/inet.h>
#import <unistd.h>
#import <fcntl.h>
#import <errno.h>

@interface EchoServer : NSObject

@property(nonatomic, readonly) uint16_t port;

- (instancetype)initWithPort:(uint16_t)port;
- (BOOL)start:(NSError **)error;
- (void)stop;

@end

@implementation EchoServer {
    int _listenFD;
    NSThread *_thread;
    BOOL _running;
}

- (instancetype)initWithPort:(uint16_t)port {
    self = [super init];
    if (self) {
        _port = port;
        _listenFD = -1;
    }
    return self;
}

- (BOOL)start:(NSError **)error {
    if (_running) return YES;

    _listenFD = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFD < 0) {
        if (error) *error = [NSError errorWithDomain:NSPOSIXErrorDomain
                                                code:errno
                                            userInfo:nil];
        return NO;
    }

    int reuse = 1;
    setsockopt(_listenFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(_port);

    if (bind(_listenFD, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        listen(_listenFD, 16) < 0) {
        if (error) *error = [NSError errorWithDomain:NSPOSIXErrorDomain
                                                code:errno
                                            userInfo:nil];
        close(_listenFD);
        _listenFD = -1;
        return NO;
    }

    struct sockaddr_in boundAddress = {0};
    socklen_t length = sizeof(boundAddress);
    getsockname(_listenFD, (struct sockaddr *)&boundAddress, &length);
    _port = ntohs(boundAddress.sin_port);

    _running = YES;
    _thread = [[NSThread alloc] initWithTarget:self
                                      selector:@selector(run)
                                        object:nil];
    [_thread start];

    return YES;
}

- (void)run {
    while (_running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(_listenFD, &readSet);

        struct timeval timeout = {0, 100000};
        int result = select(_listenFD + 1, &readSet, NULL, NULL, &timeout);
        if (result <= 0) continue;

        int clientFD = accept(_listenFD, NULL, NULL);
        if (clientFD < 0) continue;

        [self echoClient:clientFD];
        close(clientFD);
    }
}

- (void)echoClient:(int)clientFD {
    char buffer[4096];

    for (;;) {
        ssize_t received = recv(clientFD, buffer, sizeof(buffer), 0);
        if (received <= 0) break;

        ssize_t sent = 0;
        while (sent < received) {
            ssize_t result = send(clientFD,
                                  buffer + sent,
                                  (size_t)(received - sent),
                                  0);
            if (result <= 0) return;
            sent += result;
        }
    }
}

- (void)stop {
    if (!_running) return;

    _running = NO;
    if (_listenFD >= 0) {
        shutdown(_listenFD, SHUT_RDWR);
        close(_listenFD);
        _listenFD = -1;
    }

    while (_thread && ![_thread isFinished]) {
        [NSThread sleepForTimeInterval:0.001];
    }
    _thread = nil;
}

- (void)dealloc {
    [self stop];
}

@end
