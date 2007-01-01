#!/usr/bin/perl

# For each KMail Identity convert the "PGP Identity" entry (which contains a
# user ID) to a "Default PGP Key" entry (which contains a key ID)

$DEBUG = 0;

while(<>)
{
  if( /\[(Identity.*)\]/ )
  {
    $section = $1;
    next;
  }
  if( /^PGP Identity=(.*)$/ )
  {
    print STDERR "\n[$section]PGP Identity=$1\n" if ( $DEBUG );

    if( ( $1 ne "" ) &&
        ( open GnuPG, "gpg --list-secret-keys --utf8-strings '$1' |" ) )
    {
      while (<GnuPG>)
      {
        # search in gpg's output for the key id of the first matching key
        if(/^sec[^\/]*\/([0-9A-F]*)/)
        {
          print "[$section]\nDefault PGP Key=$1\n";
          last;
        }
      }
    }
    print "# DELETE [$section]PGP Identity\n";
  }
}
