#!/usr/bin/perl

use strict;
use warnings;

use Getopt::Std;

my $usage = <<"USAGE";
Usage: patcher.pl [ipmewh]
Options:
    -i      set input file name [required]
    -o      set out file name [default: STDOUT]
    -m      set mask in format {what:on_what:input_python_module} splitted by '*' [default: no mask]
    -e      set extra path variable splitted by ',' [default: no extra path]
    -w      set work dir [default no work dir]
    -h      show this help
USAGE

getopt('i:o:m:he:w:', \my %opts);

if (exists $opts{h}) {
    print STDERR $usage;
    exit 0;
}

exit 1 unless $opts{i};
exit 2 unless -f $opts{i};

open my $inf, '<', $opts{i} or exit 3;

my $outf;
if ($opts{o}) {
    open $outf, '>', $opts{o} or exit 3;
} else {
    $outf = *STDOUT;
}

my %mask;
if ($opts{m}) {
    for ($opts{m} =~ /[^*]+/g) {
        my @m = $_ =~ /[^:]+/g;
        $mask{$m[0]} = { text => $m[1], module => $m[2] || ''};
    }
}

my $done = 0;
while (my $str = <$inf>) {
    next unless $str;

    for (keys %mask) {
        if ($mask{$_}->{module}) {
            $str =~ s/(\b$_\s*=\s*)([^(]+)/$1$mask{$_}->{text}/g;
        } else {
            $str =~ s/\b$_\b/$mask{$_}->{text} || ""/eg;
        }
    }
    if (!$done && $str =~ /^from\s/) {
        if ($opts{e} or $opts{w}) {
            print $outf "import sys\n\n";
            if ($opts{w}) {
                print $outf "sys.path.append('$opts{w}')\n";
            } else {
                $opts{w} = '.';
            }
            if ($opts{e}) {
                print $outf "sys.path.append('$opts{w}/$_')\n" for $opts{e} =~ /[^,]+/g;
            }
        }
        for (keys %mask) {
            print $outf "\nfrom $mask{$_}->{module} import $mask{$_}->{text}" if $mask{$_}->{module};
        }
        print $outf "\n\n";
        $done = 1;
    }
    print $outf $str;
}
