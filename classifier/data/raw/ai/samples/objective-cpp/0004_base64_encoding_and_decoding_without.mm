#import <Foundation/Foundation.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

static const char kBase64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int Base64Value(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return (int)(c - 'A');
    if (c >= 'a' && c <= 'z') return (int)(c - 'a') + 26;
    if (c >= '0' && c <= '9') return (int)(c - '0') + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

NSString *Base64Encode(NSData *data) {
    if (data == nil) return nil;

    size_t inputLength = data.length;
    size_t groups = inputLength / 3;
    if (inputLength % 3 != 0) {
        if (groups == SIZE_MAX) return nil;
        groups++;
    }

    if (groups > SIZE_MAX / 4) return nil;
    size_t outputLength = groups * 4;

    NSMutableData *output = [NSMutableData dataWithLength:outputLength];
    if (output == nil && outputLength != 0) return nil;

    const unsigned char *input = data.bytes;
    unsigned char *encoded = output.mutableBytes;
    size_t inputIndex = 0;
    size_t outputIndex = 0;

    while (inputIndex < inputLength) {
        size_t remaining = inputLength - inputIndex;
        uint32_t a = input[inputIndex++];
        uint32_t b = remaining > 1 ? input[inputIndex++] : 0;
        uint32_t c = remaining > 2 ? input[inputIndex++] : 0;
        uint32_t value = (a << 16) | (b << 8) | c;

        encoded[outputIndex++] = kBase64Alphabet[(value >> 18) & 0x3F];
        encoded[outputIndex++] = kBase64Alphabet[(value >> 12) & 0x3F];
        encoded[outputIndex++] = remaining > 1 ? kBase64Alphabet[(value >> 6) & 0x3F] : '=';
        encoded[outputIndex++] = remaining > 2 ? kBase64Alphabet[value & 0x3F] : '=';
    }

    return [[NSString alloc] initWithData:output encoding:NSASCIIStringEncoding];
}

NSData *Base64Decode(NSString *string) {
    if (string == nil) return nil;

    NSData *asciiData = [string dataUsingEncoding:NSASCIIStringEncoding
                              allowLossyConversion:NO];
    if (asciiData == nil) return nil;

    size_t length = asciiData.length;
    if (length % 4 != 0) return nil;

    const unsigned char *input = asciiData.bytes;
    size_t groups = length / 4;
    size_t padding = 0;

    if (length != 0) {
        if (input[length - 1] == '=') padding++;
        if (length > 1 && input[length - 2] == '=') padding++;
    }

    if (groups > SIZE_MAX / 3) return nil;
    size_t outputLength = groups * 3;
    if (padding > outputLength) return nil;
    outputLength -= padding;

    NSMutableData *output = [NSMutableData dataWithLength:outputLength];
    if (output == nil && outputLength != 0) return nil;

    unsigned char *decoded = output.mutableBytes;
    size_t outputIndex = 0;

    for (size_t i = 0; i < length; i += 4) {
        bool lastGroup = (i + 4 == length);
        unsigned char c0 = input[i];
        unsigned char c1 = input[i + 1];
        unsigned char c2 = input[i + 2];
        unsigned char c3 = input[i + 3];

        int v0 = Base64Value(c0);
        int v1 = Base64Value(c1);

        if (v0 < 0 || v1 < 0) return nil;
        if (c0 == '=' || c1 == '=') return nil;

        bool pad2 = c2 == '=';
        bool pad3 = c3 == '=';

        if (!lastGroup && (pad2 || pad3)) return nil;
        if (pad2 && !pad3) return nil;
        if (pad2 && (v1 & 0x0F) != 0) return nil;

        int v2 = pad2 ? 0 : Base64Value(c2);
        int v3 = pad3 ? 0 : Base64Value(c3);

        if (v2 < 0 || v3 < 0) return nil;
        if (pad3 && (v2 & 0x03) != 0) return nil;

        uint32_t value = ((uint32_t)v0 << 18) |
                         ((uint32_t)v1 << 12) |
                         ((uint32_t)v2 << 6) |
                         (uint32_t)v3;

        if (outputIndex < outputLength)
            decoded[outputIndex++] = (value >> 16) & 0xFF;
        if (!pad2 && outputIndex < outputLength)
            decoded[outputIndex++] = (value >> 8) & 0xFF;
        if (!pad3 && outputIndex < outputLength)
            decoded[outputIndex++] = value & 0xFF;
    }

    return output;
}
