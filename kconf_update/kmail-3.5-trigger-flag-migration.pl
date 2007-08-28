#!/usr/bin/perl
#
# Copyright (c) 2007 Volker Krause <vkrause@kde.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
#

$currentGroup = "";

$source = $ARGV[0];

while (<STDIN>) {
    chomp;
    next if /^$/;
    next if /^\#/;

    # recognize groups:
    if ( /^\[(.+)\]$/ ) {
        $currentGroup = $_;
        next;
    };

    ($key,$value) = split /=/;
    next if $key eq "";

    if ( $currentGroup =~ /^\[Folder/ ) {
        if( ($key eq "UploadAllFlags" or $key eq "StatusChangedLocally") and not $value eq "true" ) {
            $value = "true";
            print "#DELETE $currentGroup $key\n";
            print "$currentGroup\n$key=$value\n"
        }
    }
}
