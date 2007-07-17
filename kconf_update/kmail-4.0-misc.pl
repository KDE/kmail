#!/usr/bin/perl -w

use strict;

# read the whole config file
my $currentGroup = "";
my %configFile;
while ( <> ) {
    chomp;
    next if ( /^$/ ); # skip empty lines
    next if ( /^\#/ ); # skip comments
    if ( /^\[/ ) { # group begin
	$currentGroup = $_;
	next;
    } elsif ( $currentGroup ne "" ) { # normal entry
	my ($key,$value) = split /=/;
	$configFile{$currentGroup}{$key}=$value;
    }
}

# filter out account groups
my @accountGroups = grep { /^\[Account \d+\]/ } keys %configFile;

# create new account type
my $newType = 30;
foreach my $accountGroup (@accountGroups) {
    my $oldType = $configFile{$accountGroup}{'Type'};
    if ( $oldType eq "pop" ) {
        $newType = "6";
    }
    if ( $oldType eq "cachedimap" ) {
        $newType = "4";
    }
    if ( $oldType eq "local" ) {
        $newType = "5";
    }
    if ( $oldType eq "maildir" ) {
        $newType = "2";
    }
    if ( $oldType eq "imap" ) {
        $newType = "0";
    }

    # change the type entry this account
    print "# DELETE ${accountGroup}Type\n";
    print "${accountGroup}\nType=$newType\n";
}