#include <stdio.h>
#include <stddef.h>

void xor_stream_cipher(unsigned char *data,
                       size_t data_length,
                       const unsigned char *key,
                       size_t key_length)
{
    if (data == NULL || key == NULL || key_length == 0) {
        return;
    }

    for (size_t i = 0; i < data_length; i++) {
        size_t key_index = i % key_length;
        data[i] ^= key[key_index];
    }
}

int main(void)
{
    unsigned char message[] = "Hello, world!";
    const unsigned char key[] = "rotate";

    size_t message_length = sizeof(message) - 1;
    size_t key_length = sizeof(key) - 1;

    printf("Original: %s\n", message);

    xor_stream_cipher(message, message_length, key, key_length);
    printf("Encrypted bytes: ");

    for (size_t i = 0; i < message_length; i++) {
        printf("%02X ", message[i]);
    }
    printf("\n");

    xor_stream_cipher(message, message_length, key, key_length);
    printf("Decrypted: %s\n", message);

    return 0;
}
