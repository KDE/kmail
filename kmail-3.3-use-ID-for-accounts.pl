#!/usr/bin/perl -w

use strict;

# this script goes through all the config keys that deal with
# accounts and replaces accounts referenced by name to be referenced
# by IDs. 
# It also renames the toplevel-folder and the sent/trash/drafts folder
# accordingly and moves the lokal folder-cache

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
    } elsif ( $currentGroup =~ /^\[Account/ and /^id/ ) {
	  # We need to prevent this script from running twice, since it
      # would change IDs of accounts then.
      # Presence of a id key in an Account section is the
	  # best indicator
      exit;
    } elsif ( $currentGroup ne "" ) { # normal entry
      my ($key,$value) = split /=/;
      $configFile{$currentGroup}{$key}=$value;
    }
}

# filter out account groups
my @accountGroups = grep { /^\[Account \d+\]/ } keys %configFile;

# create IDs for each account
my %nameToID;
foreach my $accountGroup (@accountGroups) {
  my $id;
  do {
    $id = int(rand 0x7fFFffFF);
  } while ($id <= 0);
  my $name = $configFile{$accountGroup}{'Name'};
  $nameToID{$name} = $id;
  # create the uoid entries of [Identity #n] groups:
  print "${accountGroup}\nid=$id\n";
  # rename the trash
  my $trash = $configFile{$accountGroup}{'trash'};
  if (&replaceID($trash)) {
    print "# DELETE ".$accountGroup."trash\n";
    print "${accountGroup}\ntrash=".&replaceID($trash)."\n";
  }
}

# we need the directory where the imap cache is stored
my $basedir = "`kde-config --localprefix`share/apps/kmail/imap";

# Now, go through all [Folder-*] groups that belong to (d)imap folders
# and replace the account name with the id
my @folderGroups = grep { /^\[Folder-.*\]/ } keys %configFile;

foreach my $folderGroup ( @folderGroups ) 
{
  my $isRootFolder = 1;
  # extract the accountname
  my (@parts) = split (/\[Folder-/,$folderGroup);
  my $account = substr($parts[1], 0, -1);
  if ($account =~ /\.directory/)
  {
    # .account.directory
    my (@dirparts) = split (/\./,$account);
    $account = $dirparts[1];
    # this is no root folder
    $isRootFolder = 0;
  }
  # write the new entry
  if ( exists( $nameToID{$account} ) ) 
  {
    print "# DELETEGROUP $folderGroup\n";
    my $folderGroupNew = $folderGroup;
    $folderGroupNew =~ s/$account/$nameToID{$account}/;
    # new account section
    print "$folderGroupNew\n";
    # print all original keys
    my %groupData = %{$configFile{$folderGroup}};
    foreach my $key ( keys %groupData ) {
      print "$key=" . $groupData{$key} . "\n";
    }
    if ($isRootFolder) {
      print "Label=$account\n";
    }

    # move the directory
    my $systemcall = "mv ${basedir}/\.${account}\.directory ${basedir}/\.".$nameToID{$account}."\.directory";
    system($systemcall);
    $systemcall = "mv ${basedir}/${account} ${basedir}/".$nameToID{$account};
    system($systemcall);
  }
}

# go through all identities and replace the sent-mail and drafts folder
my @identities = grep { /^\[Identity #\d\]/ } keys %configFile;

foreach my $identity (@identities)
{
  my $drafts = $configFile{$identity}{'Drafts'};
  my $sent = $configFile{$identity}{'Fcc'};
  # extract the account name
  if (&replaceID($drafts))
  {
    print "# DELETE ".$identity."Drafts\n";
    print "${identity}\nDrafts=".&replaceID($drafts)."\n";
  }
  if (&replaceID($sent))
  {
    print "# DELETE ".$identity."Fcc\n";
    print "${identity}\nFcc=".&replaceID($sent)."\n";
  }
}

# and finally the startupFolder
my $startup = $configFile{'[General]'}{'startupFolder'};
if (&replaceID($startup)) {
  print "# DELETE [General]startupFolder\n";
  print "[General]\nstartupFolder=".&replaceID($startup)."\n";
}

## Returns input string with replaced account name
sub replaceID
{
  my ($input) = @_;

  if ($input =~ /\.directory/)
  {
    my (@dirparts) = split (/\./,$input);
    my $account = $dirparts[1];
    if ( exists( $nameToID{$account} ) ) 
    {
      $input =~ s/$account/$nameToID{$account}/;
      return $input;
    }
  }
}
