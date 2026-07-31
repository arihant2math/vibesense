use strict;
use warnings;

sub xor_stream_cipher {
    my ($data, $key) = @_;

    die "Key must not be empty\n" unless length $key;

    my $result = '';
    my $key_length = length $key;

    for my $i (0 .. length($data) - 1) {
        my $data_byte = ord(substr($data, $i, 1));
        my $key_byte  = ord(substr($key, $i % $key_length, 1));

        $result .= chr($data_byte ^ $key_byte);
    }

    return $result;
}

my $plaintext = "Hello, world!";
my $key = "secret";

my $ciphertext = xor_stream_cipher($plaintext, $key);
my $decrypted  = xor_stream_cipher($ciphertext, $key);

print "Ciphertext: $ciphertext\n";
print "Decrypted: $decrypted\n";
