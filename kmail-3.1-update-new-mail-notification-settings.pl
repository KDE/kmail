#!/usr/bin/perl -w

use strict;

my $presentation = -1;
my $commandline = "";
while ( <> ) {
  chomp;
  if ( /^msgbox-on-mail/ ) {
    my ($key,$value) = split /=/;
    if( -1 == $presentation ) {
      $presentation = 0;
    }
    $presentation += 2*( $value eq "true" );
  }
  elsif ( /^exec-on-mail-cmd/ ) {
    my ($key,$value) = split /=/;
    $commandline = $value;
  }
  elsif ( /^exec-on-mail/ ) {
    my ($key,$value) = split /=/;
    if( -1 == $presentation ) {
      $presentation = 0;
    }
    $presentation += 32*( $value eq "true" );
  }
}

if ( "" ne $commandline ) {
  print "commandline=$commandline\n";
}
if ( -1 != $presentation ) {
  print "presentation=$presentation\n";
}

print "# DELETE msgbox-on-mail\n";
print "# DELETE exec-on-mail\n";
print "# DELETE exec-on-mail-cmd\n";
