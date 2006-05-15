#!/usr/bin/perl -w

use strict;

# this script goes through all the config keys that deal with
# accounts and replaces accounts referenced by name to be referenced
# by IDs. 
# It also renames the toplevel-folder and the sent/trash/drafts folder
# accordingly and renames all references
# last but not least we move the lokal folder-cache of (d)imap folders

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
    } elsif ( $currentGroup =~ /^\[Account/ and /^Id/ ) {
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
my %nameToType;
foreach my $accountGroup (@accountGroups) {
  my $id;
  do {
    $id = int(rand 0x7fFFffFF);
  } while ($id <= 0);
  my $name = $configFile{$accountGroup}{'Name'};
  # remember id and type
  $nameToID{$name} = $id;
  $nameToType{$name} = $configFile{$accountGroup}{'Type'};
  # create the id entries
  print "${accountGroup}\nId=$id\n";
}

foreach my $accountGroup (@accountGroups) {
  # rename the trash
  my $trash = $configFile{$accountGroup}{'trash'};
  if (&replaceID($trash)) {
    print "# DELETE ".$accountGroup."trash\n";
    print "${accountGroup}\ntrash=".&replaceID($trash)."\n";
  }
}

# we need the directory where the imap cache is stored
open(CMD, "kde-config --localprefix|");
my $basedir = <CMD>;
chomp( $basedir );
$basedir = $basedir."share/apps/kmail";

# Now, go through all [Folder-*] groups that belong to (d)imap folders
# and replace the account name with the id
my @folderGroups = grep { /^\[Folder-.*\]/ } keys %configFile;

foreach my $folderGroup ( @folderGroups ) 
{
  my $isRootFolder = 1;
  # extract the accountname
  my (@parts) = split (/\[Folder-/,$folderGroup);
  my $account = substr($parts[1], 0, -1);
  if ($account =~ /^[^\/]*\.directory\//)
  {
    # .account.directory
    my (@dirparts) = split (/\.directory\//,$account);
    $account = substr( $dirparts[0], 1 );
    # this is no root folder
    $isRootFolder = 0;
  }
  # delete the old group and write the new entry
  my $accountDecoded = QFileDecode( $account );
  if ( exists( $nameToID{$accountDecoded} ) )
  {
    my $id = $nameToID{$accountDecoded};
    print "# DELETEGROUP $folderGroup\n";
    my $folderGroupNew = $folderGroup;
    my $pattern = quotemeta( $account );
    $folderGroupNew =~ s/$pattern/$id/;
    # new account section
    print "$folderGroupNew\n";
    # print all original keys
    my %groupData = %{$configFile{$folderGroup}};
    foreach my $key ( keys %groupData ) {
      print "$key=" . $groupData{$key} . "\n";
    }
    if ($isRootFolder) {
      # new label and id of this rootfolder
      print "SystemLabel=$account\n";
      print "Id=".$id."\n";

      # move the directory
      my $subdir;
      if ($nameToType{$accountDecoded} eq "imap") {
        $subdir = "imap";
      } elsif ($nameToType{$accountDecoded} eq "cachedimap") {
        $subdir = "dimap";
      }
      my $oldname = QFileEncode( "$basedir/$subdir/\.$account\.directory" );
      my $systemcall = "mv '$oldname' '$basedir/$subdir/\.".$id."\.directory'";
      system($systemcall);

      $oldname = QFileEncode( "$basedir/$subdir/$account" );
      $systemcall = "mv '$oldname' '$basedir/$subdir/".$id."'";
      system($systemcall);

      $oldname = QFileEncode( "$basedir/$subdir/\.$account\.index" );
      $systemcall = "mv '$oldname' '$basedir/$subdir/\.".$id."\.index'";
      system($systemcall);

      $oldname = QFileEncode( "$basedir/$subdir/\.$account\.index.ids" );
      $systemcall = "mv '$oldname' '$basedir/$subdir/\.".$id."\.index.ids'";
      system($systemcall);
    }
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

# go through all filters and replace the target
my @filterGroups = grep { /^\[Filter \#\d+\]/ } keys %configFile;
foreach my $filterGroup (@filterGroups) 
{
    my $numActions = +$configFile{$filterGroup}{'actions'};
    # go through all actions in search for "set identity":
    for ( my $i = 0 ; $i < $numActions ; ++$i ) 
    {
      my $actionName = "action-name-$i";
      my $actionArgs = "action-args-$i";
      if ( $configFile{$filterGroup}{$actionName} eq "transfer" &&
           &replaceID($configFile{$filterGroup}{$actionArgs}) ) 
      {
        print "# DELETE $filterGroup$actionArgs\n";
        print "$filterGroup\n$actionArgs=".
          &replaceID($configFile{$filterGroup}{$actionArgs})."\n";
      }
    }
}

# previous fcc
my $pfcc = $configFile{'[Composer]'}{'previous-fcc'};
if (&replaceID($pfcc)) {
  print "# DELETE [Composer]previous-fcc\n";
  print "[Composer]\nprevious-fcc=".&replaceID($pfcc)."\n";
}

# GroupwareFolder
my $groupware = $configFile{'[Groupware]'}{'GroupwareFolder'};
if (&replaceID($groupware)) {
  print "# DELETE [Groupware]GroupwareFolder\n";
  print "[Groupware]\nGroupwareFolder=".&replaceID($groupware)."\n";
}

# and finally the startupFolder
my $startup = $configFile{'[General]'}{'startupFolder'};
if (&replaceID($startup)) {
  print "# DELETE [General]startupFolder\n";
  print "[General]\nstartupFolder=".&replaceID($startup)."\n";
}

## Returns input string with replaced account name
## If there is nothing to replace it returns undef
sub replaceID
{
  my ($input) = @_;

  if ($input && $input =~ /\.directory/)
  {
    my (@dirparts) = split (/\.directory\//,$input);
    my $account = substr( $dirparts[0], 1 );
    my $accountDecoded = QFileDecode( $account );
    if ( exists( $nameToID{$accountDecoded} ) )
    {
      my $pattern = quotemeta( $account );
      $input =~ s/$pattern/$nameToID{$accountDecoded}/;
      return $input;
    }
  }
}

## emulate QFileDecode
sub QFileDecode
{
  my ($input) = @_;

  $input =~ s/%20/ /g;
  $input =~ s/%40/\@/g;
  $input =~ s/%25/%/g;  # must be the last one

  return $input;
}

## emulate QFileEncode
sub QFileEncode
{
  my ($input) = @_;

  $input =~ s/%/%25/g;  # must be the first one
  $input =~ s/ /%20/g;
  $input =~ s/\@/%40/g;

  return $input;
}
