use strict;
use warnings;
use Time::HiRes qw(sleep);

sub retry_with_exponential_backoff {
    my (%args) = @_;

    my $operation = $args{operation} or die "operation is required";
    my $max_attempts = $args{max_attempts} // 5;
    my $base_delay = $args{base_delay} // 0.5;
    my $max_delay = $args{max_delay} // 30;
    my $jitter = $args{jitter} // 1.0;

    my $attempt = 0;
    my $last_error;

    while ($attempt < $max_attempts) {
        $attempt++;

        my ($ok, @result);
        {
            local $@;
            $ok = eval {
                @result = $operation->($attempt);
                1;
            };
            $last_error = $@ unless $ok;
        }

        return wantarray ? @result : $result[0] if $ok;

        last if $attempt >= $max_attempts;

        my $delay = $base_delay * (2 ** ($attempt - 1));
        $delay = $max_delay if $delay > $max_delay;

        my $jitter_factor = 1 + (($jitter * 2 * rand()) - $jitter);
        $delay *= $jitter_factor;
        $delay = 0 if $delay < 0;

        sleep($delay);
    }

    die $last_error || "operation failed after $max_attempts attempts";
}

1;
