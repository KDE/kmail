#!/usr/bin/perl
# David Faure <faure@kde.org>
# License: GPL

$currentGroup = "";

while (<>) {
    next if /^$/;
    # recognize groups:
    if ( /^\[(.+)\]$/ ) { 
        $currentGroup = $1;
        if ( $currentGroup =~ /^Identity/ ) {
            print "# DELETEGROUP [$currentGroup]\n";
            print "[$currentGroup]\n";
        }
        next;
    };
    # Move over keys from the identity groups
    if ( $currentGroup =~ /^Identity/ ) {
        print;
    }
    # Move over the key for the default identity
    elsif ( $currentGroup eq 'General' ) {
	($key,$value) = split /=/;
	chomp $value;
        if ( $key eq 'Default Identity' ) {
            print "[$currentGroup]\n$key=$value\n";
            print "# DELETE [$currentGroup]$key\n";
        }
    }
}
