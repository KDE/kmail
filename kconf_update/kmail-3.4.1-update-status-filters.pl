#!/usr/bin/perl -w

use strict;

# This script converts lower case status filter rules to upper case.

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

# go through all filters and check for rules which are no longer valid
my @filterGroups = grep { /^\[Filter \#\d+\]/ } keys %configFile;
foreach my $filterGroup (@filterGroups) {
  my $numRules = $configFile{$filterGroup}{'rules'};
  # go through all rules:
  for ( my $i = 0; $i < $numRules; ++$i ) {
    my $c = chr( ord("A") + $i );
    my $fieldKey = "field$c";
    my $field = $configFile{$filterGroup}{$fieldKey};
    if ( $field eq "<status>" ) {
      my $contentsKey = "contents$c";
      my $contents = $configFile{$filterGroup}{$contentsKey};
      if ( $contents =~ /^[a-z]/ ) {
        $contents = ucfirst( $contents );
        print "# DELETE $filterGroup$contentsKey\n";
        print "$filterGroup\n$contentsKey=$contents\n";
      }
    }
  }
}
