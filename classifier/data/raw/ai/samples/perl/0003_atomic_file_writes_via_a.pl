use strict;
use warnings;
use File::Temp qw(tempfile);
use File::Copy qw(move);

sub atomic_write {
    my ($path, $content) = @_;

    my ($temp_fh, $temp_path) = tempfile(
        'atomic-write-XXXXXX',
        DIR    => '.',
        UNLINK => 0,
    );

    eval {
        print {$temp_fh} $content
            or die "Failed to write temporary file: $!";

        close $temp_fh
            or die "Failed to close temporary file: $!";

        move($temp_path, $path)
            or die "Failed to rename temporary file to '$path': $!";
    };

    my $error = $@;

    if ($error) {
        close $temp_fh if defined $temp_fh;
        unlink $temp_path if defined $temp_path;
        die $error;
    }
}

atomic_write("output.txt", "Hello, world!\n");
