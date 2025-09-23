#!/usr/bin/perl
use strict;
use warnings;
use POSIX qw(strftime);

my $logdir = '/run';

my $target_date = strftime('%Y-%m-%d', localtime(time - 2*24*60*60));

opendir(my $dh, $logdir) or die "Can't open $logdir: $!";
while (my $file = readdir($dh)) {
    next unless $file =~ /^log_(\d{4}-\d{2}-\d{2})\.log$/;
    my $fdate = $1;
    if ($fdate lt $target_date) {
        my $fullpath = "$logdir/$file";
        unlink $fullpath or warn "Can't delete $fullpath: $!";
    }
}
closedir($dh);
