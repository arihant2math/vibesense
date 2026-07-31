#import <Foundation/Foundation.h>

static BOOL IsMatch(NSString *text, NSString *pattern) {
    NSUInteger textLength = text.length;
    NSUInteger patternLength = pattern.length;

    NSMutableArray<NSMutableArray<NSNumber *> *> *dp = [NSMutableArray arrayWithCapacity:textLength + 1];

    for (NSUInteger i = 0; i <= textLength; i++) {
        NSMutableArray<NSNumber *> *row = [NSMutableArray arrayWithCapacity:patternLength + 1];
        for (NSUInteger j = 0; j <= patternLength; j++) {
            [row addObject:@NO];
        }
        [dp addObject:row];
    }

    dp[0][0] = @YES;

    for (NSUInteger j = 2; j <= patternLength; j++) {
        if ([pattern characterAtIndex:j - 1] == '*') {
            dp[0][j] = dp[0][j - 2];
        }
    }

    for (NSUInteger i = 1; i <= textLength; i++) {
        for (NSUInteger j = 1; j <= patternLength; j++) {
            unichar patternCharacter = [pattern characterAtIndex:j - 1];
            unichar textCharacter = [text characterAtIndex:i - 1];

            if (patternCharacter == '.' || patternCharacter == textCharacter) {
                dp[i][j] = dp[i - 1][j - 1];
            } else if (patternCharacter == '*' && j >= 2) {
                unichar precedingCharacter = [pattern characterAtIndex:j - 2];
                BOOL matches = precedingCharacter == '.' || precedingCharacter == textCharacter;

                dp[i][j] = dp[i][j - 2];
                if (matches) {
                    dp[i][j] = @([dp[i][j] boolValue] || [dp[i - 1][j] boolValue]);
                }
            }
        }
    }

    return [dp[textLength][patternLength] boolValue];
}
