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
  } elsif ( $currentGroup eq "[General]" and /^systray-on-mail/ ) {
    my ($key,$value) = split /=/;
    print "# DELETE $currentGroup$key\n";
    print "[General]\nSystemTrayEnabled=$value\n";
  } elsif ( $currentGroup eq "[General]" and /^systray-on-new/ ) {
    my ($key,$value) = split /=/;
    print "# DELETE $currentGroup$key\n";
    if ( $value eq "true" ) {
      print "[General]\nSystemTrayPolicy=ShowOnUnread\n";
    }
    else {
      print "[General]\nSystemTrayPolicy=ShowAlways\n";
    }
  } elsif ( /^MailingListPostingAddress/ ) {
    my ($key,$value) = split /=/;
    if ( not $value eq "" and not $value =~ /^mailto:/ ) {
      print "# DELETE $currentGroup$key\n";
      print "$currentGroup\n$key=mailto:$value\n";
    }
  }
}
