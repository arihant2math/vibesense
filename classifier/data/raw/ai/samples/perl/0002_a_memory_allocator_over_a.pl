#!/usr/bin/env perl
use strict;
use warnings;
use bytes;

package FixedArena;

sub new {
    my ($class, $size) = @_;
    die "Arena size must be positive\n" unless $size > 0;

    return bless {
        arena     => "\0" x $size,
        size      => $size,
        free      => { 0 => $size },
        allocated => {},
    }, $class;
}

sub _align {
    my ($size) = @_;
    return ($size + 7) & ~7;
}

sub malloc {
    my ($self, $size) = @_;
    die "Allocation size must be positive\n" unless defined($size) && $size > 0;

    my $need = _align($size);

    for my $offset (sort { $a <=> $b } keys %{ $self->{free} }) {
        my $available = $self->{free}{$offset};
        next if $available < $need;

        delete $self->{free}{$offset};

        if ($available > $need) {
            $self->{free}{ $offset + $need } = $available - $need;
        }

        $self->{allocated}{$offset} = $need;
        return $offset;
    }

    return undef;
}

sub free {
    my ($self, $ptr) = @_;
    die "Invalid pointer\n" unless exists $self->{allocated}{$ptr};

    my $size = delete $self->{allocated}{$ptr};
    my $start = $ptr;
    my $end = $ptr + $size;

    for my $offset (sort { $a <=> $b } keys %{ $self->{free} }) {
        my $block_end = $offset + $self->{free}{$offset};

        if ($block_end == $start) {
            $start = $offset;
            $size += delete $self->{free}{$offset};
        }
    }

    for my $offset (sort { $a <=> $b } keys %{ $self->{free} }) {
        next if $offset != $end;
        $size += delete $self->{free}{$offset};
        last;
    }

    $self->{free}{$start} = $size;
}

sub write {
    my ($self, $ptr, $data) = @_;
    die "Invalid pointer\n" unless exists $self->{allocated}{$ptr};

    my $capacity = $self->{allocated}{$ptr};
    die "Data exceeds allocation\n" if length($data) > $capacity;

    substr($self->{arena}, $ptr, length($data), $data);
}

sub read {
    my ($self, $ptr, $length) = @_;
    die "Invalid pointer\n" unless exists $self->{allocated}{$ptr};
    die "Read exceeds allocation\n"
        if $length < 0 || $length > $self->{allocated}{$ptr};

    return substr($self->{arena}, $ptr, $length);
}

sub dump_free_list {
    my ($self) = @_;
    return [
        map { [ $_, $self->{free}{$_} ] }
        sort { $a <=> $b } keys %{ $self->{free} }
    ];
}

package main;

my $allocator = FixedArena->new(128);

my $first  = $allocator->malloc(16);
my $second = $allocator->malloc(24);
my $third  = $allocator->malloc(8);

die "Allocation failed\n" unless defined $first && defined $second && defined $third;

$allocator->write($first, "hello");
$allocator->write($second, "fixed arena");

print "First block: ", $allocator->read($first, 5), "\n";
print "Second block: ", $allocator->read($second, 11), "\n";

$allocator->free($second);
$allocator->free($first);

my $reused = $allocator->malloc(32);
print "Reused block offset: $reused\n" if defined $reused;

$allocator->free($third);
$allocator->free($reused) if defined $reused;

print "Free blocks:\n";
for my $block (@{ $allocator->dump_free_list }) {
    print "  offset=$block->[0], size=$block->[1]\n";
}
