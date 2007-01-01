#!/usr/bin/perl -w

use strict;

# This script updates some configuration keys

# read the whole config file
my $currentGroup = "";
my %configFile;
while ( <> ) {
  chomp; # eat the trailing '\n'
  next if ( /^$/ ); # skip empty lines
  next if ( /^\#/ ); # skip comments
  if ( /^\[/ ) { # group begin
    $currentGroup = $_;
    next;
  } elsif ( $currentGroup eq "[Behaviour]" and /^JumpToUnread/ ) {
    my ($key,$value) = split /=/;
    print "# DELETE $currentGroup$key\n";
    if ( $value eq "true" ) {
      print "[Behaviour]\nActionEnterFolder=SelectFirstUnreadNew\n";
    }
    else {
      print "[Behaviour]\nActionEnterFolder=SelectFirstNew\n";
    }
  }
}
