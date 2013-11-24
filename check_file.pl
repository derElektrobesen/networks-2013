#!/usr/bin/perl

use strict;
use warnings;
use Digest::MD5;

my $home = "/tmp/course_prj";
my @names = ("$home/before", "$home/output_data", "$home/srv_cache_get", "$home/input_data", "$home/read_file_piece");

my $test_file_name = 'test';

sub count_md5 {
    open my $f, "<", shift;
    binmode $f;

    my $md5 = Digest::MD5->new;
    while (<$f>) {
        $md5->add($_);
    }
    return $md5->hexdigest;
}

print "Default file sum: " . count_md5("$home/downloads/$test_file_name") . "\n";
print "Result file sum:  " . count_md5("$home/$test_file_name") . "\n\n";

for my $name (@names) {
    unless (-f $name) {
        print "file '$name' not found\n";
        next;
    }

    my $i = 0;
    open my $f, "<", $name;
    while (<$f>) {
        print "found at line $i at file $name\n" if /[^0-9\n]/g;
        $i++;
    }
}
