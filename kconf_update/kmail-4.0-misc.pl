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
my $newType;
foreach my $accountGroup (@accountGroups) {
    my $oldType = $configFile{$accountGroup}{'Type'};
    $newType = $oldType;
    if ( $oldType eq "pop" or $oldType eq "6" ) {
        $newType = "Pop";
    }
    if ( $oldType eq "cachedimap" or $oldType eq "4" ) {
        $newType = "DImap";
    }
    if ( $oldType eq "local" or $oldType eq "5" ) {
        $newType = "Local";
    }
    if ( $oldType eq "maildir" or $oldType eq "2" ) {
        $newType = "Maildir";
    }
    if ( $oldType eq "imap" or $oldType eq "0" ) {
        $newType = "Imap";
    }

    # change the type entry this account
    print "# DELETE ${accountGroup}Type\n";
    print "${accountGroup}\nType=$newType\n";
}