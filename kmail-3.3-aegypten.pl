#!/usr/bin/perl -w

use strict;

# This script updates some configuration keys and adds a 'mailto:' to all 
# mailing list posting addresses

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
  } elsif ( $currentGroup eq "" and /^pgpType/ ) {
    my ($key,$value) = split /=/;
    if ( $value != 0 ) {
      my @newvalues = ("","Kpgp/gpg1","Kpgp/pgp v2","Kpgp/pgp v5","Kpgp/pgp v6","");
      print "OpenPGP=$newvalues[$value]\n";
    }
  }
}
