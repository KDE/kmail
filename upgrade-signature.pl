#!/usr/bin/perl

my (%data);
my $section = "";

sub process {
    # delete obsolete keys:
    print "# DELETE [$section]UseSignatureFile\n";

    # now determine the type of signature:
    if ( $data{'usefile'} =~ /false/i ) {
	# type = inline
	if ( $data{'inline'} ne "" ) {
	    print "[$section]\nSignature Type=inline\n";
	} else {
	    print "[$section]\nSignature Type=disabled\n";
	    print "# DELETE [$section]Inline Signature\n";
	}
	print "# DELETE [$section]Signature File\n";
    } else {
	# type = file or command
	if ( $data{'file'} =~ /\|$/ ) {
	    # a trailing pipe means:
	    # type = command
	    chop $data{'file'};
	    print "[$section]\nSignature Type=command\n";
	    print "[$section]\nSignature Command=", $data{'file'}, "\n";
	    print "# DELETE [$section]Signature File\n";
	    print "# DELETE [$section]Inline Signature\n";
	} elsif ( $data{'file'} eq "" ) {
	    # empty filename means:
	    # type = disabled
	    print "[$section]\nSignature Type=disabled\n";
	    print "# DELETE [$section]Inline Signature\n";
	    print "# DELETE [$section]Signature File\n";
	} else {
	    # type = file
	    print "[$section]\nSignature Type=file\n";
	    print "# DELETE [$section]Inline Signature\n";
	}
    }
}

#loop over all lines to find Identity sections:
while (<>) {
    if ( /\[(Identity[^]]*)\]/ ) {
        # new group means that we have to process the old group:
        if ( $section ne "" ) { process(); }
	$section = $1;
	%data = ();
	next;
    }
    chomp;
    # We need to prevent this script from begin run twice
    # since it would set all signatures to 'disabled' then.
    # Presence of the Signature Type key is the best indicator.
    /^Signature Type/ and exit;
    /^Inline Signature=(.*)$/ and $data{'inline'} = $1;
    /^Signature File=(.*)$/   and $data{'file'} = $1;
    /^UseSignatureFile=(.*)$/ and $data{'usefile'} = $1;
}
#and don't forget to preocess the last group ;-)
if ( $section ne "" ) { process(); }
