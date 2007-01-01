#!/usr/bin/perl

my (%data);

$currentGroup = "";

while (<>) {
    next if /^$/;
    # filter out groups:
    if ( /^\[(.+)\]$/ ) { $currentGroup = $1; next; };
    # store all keys regarding Identities in the hash:
    if ( $currentGroup =~ /^Identity/ ) {
	($key,$value) = split /=/;
	chomp $value;
	$data{$currentGroup}{$key} = $value;
    }
}

# We need to prevent this script from being run twice, since it would
# kill all identities then.
# Non-presence of the [Identity]IdentityList key is the best indiator:
unless ( defined( $data{'Identity'}{'IdentityList'} ) ) { exit; }

# first, delete all old groups:
foreach $group ( keys %data ) {
    print "# DELETEGROUP [$group]\n";
}

# now, extract the list of valid identities (and their sequence):
$rawIdentityList = $data{'Identity'}{'IdentityList'};
# don't include the IdentityList anymore:
delete $data{'Identity'}{'IdentityList'};
# remove backslash-quoting:
$rawIdentityList =~ s/\\(.)/$1/g;
# split into a list at unquoted commas:
@identities = split /(?<!\\),/, $rawIdentityList;
# unquote individual items yet again:
for ( @identities ) { s/\\(.)/$1/g; }
# build the list of groups (this time incl. the default identity):
@groups = ( 'Identity', map { $_ = "Identity-$_"; } @identities );
# write out the groups, now named "Identity #n",
# with the same data and the same keys that the old groups had:
$n = 0;
foreach $group (@groups) {
    %groupData = %{$data{$group}};
    print "[Identity #$n]\n";
    foreach $key ( keys %groupData ) {
	print "$key=" 
	    . $groupData{$key} . "\n";
    }
    $n++;
}
# remember which one is default:
print "[General]\nDefault Identity=" . $data{'Identity'}{'Identity'} . "\n";
