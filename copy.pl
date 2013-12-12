#!/usr/bin/perl

use strict;
use warnings;
use Storable;
use File::Basename;

my $HELP = <<HELP;
    copy.pl -- copy current git repo into remote host
    Usage: copy.pl [-params] [files]

    Parametrs [defaults is into copy_params.dump]:
        -n  -- set hostname
        -u  -- set user
        -i  -- set identity rsa file
        -w  -- set work directory [default .]
        -d  -- set destination directory
        -h  -- show this help
HELP

my %params = (
    hostname    => '',
    user        => '',
    ident       => '',
    work_dir    => '.',
    dest_dir    => '',
);

my $fname = 'copy_params.dump';

if (-f $fname) {
    %params = %{ retrieve $fname };
}

my @files;
my $next = 0;

for (@ARGV) {
    if ($_ eq '-h') {
        print $HELP;
        exit 0;
    } elsif ($_ eq '-n') {
        $next = 1;
    } elsif ($_ eq '-u') {
        $next = 2;
    } elsif ($_ eq '-i') {
        $next = 3;
    } elsif ($_ eq '-w') {
        $next = 4;
    } elsif ($_ eq '-d') {
        $next = 5;
    } elsif ($next == 1) {
        $params{hostname} = $_;
    } elsif ($next == 2) {
        $params{user} = $_;
    } elsif ($next == 3) {
        $params{ident} = $_;
    } elsif ($next == 4) {
        $params{work_dir} = $_;
    } elsif ($next == 5) {
        $params{dest_dir} = $_;
    } else {
        push @files, $_;
    }
}

die "Not all params given\n" unless $params{hostname} && $params{user} && $params{ident} && $params{dest_dir};

store \%params, $fname;

chroot $params{work_dir};

unless (@files) {
    open my $f, "git ls-tree --name-only -r HEAD |";
    while (<$f>) {
        chomp;
        push @files, $_;
    }
}

my %dirs;
for (@files) {
    push @{$dirs{dirname $_}}, $_;
}

for (keys %dirs) {
    print "\nscp -i $params{ident} @{$dirs{$_}} $params{user}\@$params{hostname}:$params{dest_dir}/$_\n";
    system "scp", "-i", $params{ident}, @{$dirs{$_}}, "$params{user}\@$params{hostname}:$params{dest_dir}/$_";
}
system "ssh", "-i", $params{ident}, "$params{user}\@$params{hostname}", "cd $params{dest_dir}; make";
