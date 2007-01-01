#!/usr/bin/perl -w

use strict;

# this script goes through all the config keys that deal with
# identities and replaces UOID 0 (which was erroneously always assigned to the
# first identity) with a non-zero UOID.

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

# filter out identity groups
my @identityGroups = grep { /^\[Identity \#\d+\]/ } keys %configFile;

# create new UOID for an identity with UOID 0
my $newUoid = 0;
foreach my $identityGroup (@identityGroups) {
    my $oldUoid = $configFile{$identityGroup}{'uoid'};
    if ( $oldUoid eq "0" ) {
        $newUoid = int(rand 0x7fFFffFE) + 1;
        # change the uoid entry this identity
        print "# DELETE ${identityGroup}uoid\n";
        print "${identityGroup}\nuoid=$newUoid\n";
    }
}

if ( $newUoid != 0 ) {
    # change the default identity value if it was 0
    my $tempId = $configFile{'[General]'}{'Default Identity'};
    if ( $tempId eq "0" ) {
        print "# DELETE [General]Default Identity\n";
        print "[General]\nDefault Identity=$newUoid\n";
    }

    # [Composer]previous-identity
    $tempId = $configFile{'[Composer]'}{'previous-identity'};
    if ( $tempId eq "0" ) {
        print "# DELETE [Composer]previous-identity\n";
        print "[Composer]\nprevious-identity=$newUoid\n";
    }

    # Now, go through all [Folder-*] groups and replace all occurrences of
    # Identity=0 with Identity=$newUoid
    my @folderGroups = grep { /^\[Folder-.*\]/ } keys %configFile;

    foreach my $folderGroup ( @folderGroups ) {
        $tempId = $configFile{$folderGroup}{'Identity'};
        if ( $tempId eq "0" ) {
            print "# DELETE ${folderGroup}Identity\n";
            print "$folderGroup\nIdentity=$newUoid\n";
        }
    }

    # Now, go through all [Filter #n] groups and replace all occurrences of
    # UOID 0 as argument to the 'set identity' filter action with the new UOID
    my @filterGroups = grep { /^\[Filter \#\d+\]/ } keys %configFile;

    foreach my $filterGroup (@filterGroups) {
        my $numActions = +$configFile{$filterGroup}{'actions'};
        # go through all actions in search for "set identity":
        for ( my $i = 0 ; $i < $numActions ; ++$i ) {
	    my $actionName = "action-name-$i";
            my $actionArgs = "action-args-$i";
            if ( $configFile{$filterGroup}{$actionName} eq "set identity" ) {
                # found one:
                # replace it's argument with the new UOID if necessary
                $tempId = $configFile{$filterGroup}{$actionArgs};;
                if ( $tempId eq "0" ) {
                    print "# DELETE $filterGroup$actionArgs\n";
                    print "$filterGroup\n$actionArgs=$newUoid\n";
                }
            }
        }
    }
}
