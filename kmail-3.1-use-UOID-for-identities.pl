#!/usr/bin/perl -w

use strict;

# this script goes through all the config keys that deal with
# identities and replaces identies referenced by name to be referenced
# by UOIDs. To this end, adds uoid keys to the identity groups.

# read the whole config file:
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

# filter out identity groups:
my @identityGroups = grep { /^\[Identity \#\d+\]/ } keys %configFile;

# create UOIDs for each identity:
my %nameToUOID;
foreach my $identityGroup (@identityGroups) {
    my $uoid = int(rand 0x7fFFffFF);
    my $name = $configFile{$identityGroup}{'Identity'};
    $nameToUOID{$name} = $uoid;
    # create the uoid entries of [Identity #n] groups:
    print "${identityGroup}\nuoid=$uoid\n";
}

# change the default identity value:
print "# DELETE [General]Default Identity\n[General]\nDefault Identity="
    . $nameToUOID{$configFile{'[General]'}{'Default Identity'}} . "\n";

# [Composer]previous-identity
print "# DELETE [Composer]previous-identity\n[Composer]\nprevious-identity="
    . $nameToUOID{$configFile{'[Composer]'}{'previous-identity'}} . "\n";

# Now, go through all [Folder-*] groups and replace the Identity value
# with the UOID. Also, move MailingListIdentity entries to Identity entries:
my @folderGroups = grep { /^\[Folder-.*\]/ } keys %configFile;

foreach my $folderGroup ( @folderGroups ) {
    my $identity = "";
    # delete the (MailingList)Identity keys:
    print "# DELETE ${folderGroup}MailingListIdentity\n";
    print "# DELETE ${folderGroup}Identity\n";
    # extract the identity name:
    if ( exists ($configFile{$folderGroup}{'Identity'}) ) {
	$identity = $configFile{$folderGroup}{'Identity'};
    }
    if ( $identity eq ""
	 and exists($configFile{$folderGroup}{'MailingListIdentity'}) ) {
	$identity = $configFile{$folderGroup}{'MailingListIdentity'};
    }
    # write the new Identity=<uoid> key if identity is not empty or default:
    if ( $identity ne "" and $identity ne "Default" ) {
	print "$folderGroup\nIdentity=" . $nameToUOID{$identity} . "\n";
    }
}

# Now, go through all [Filter #n] groups and change arguments to the
# 'set identity' filter action to use UOIDs:

my @filterGroups = grep { /^\[Filter \#\d+\]/ } keys %configFile;

foreach my $filterGroup (@filterGroups) {
    my $numActions = +$configFile{$filterGroup}{'actions'};
    # go through all actions in search for "set identity":
    for ( my $i = 0 ; $i < $numActions ; ++$i ) {
	my $actionName = "action-name-$i";
	my $actionArgs = "action-args-$i";
	if ( $configFile{$filterGroup}{$actionName} eq "set identity" ) {
	    # found one: replace it's argument with the UOID:
	    print "# DELETE $filterGroup$actionArgs\n$filterGroup\n$actionArgs="
		. $nameToUOID{$configFile{$filterGroup}{$actionArgs}} . "\n";
	}
    }
}

