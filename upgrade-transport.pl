#!/usr/bin/perl
my (%data);

#read in all the data and split it up into hashes.
while (<>) {
  $data{$1} = $2 if /^([^=]*)=(.*)$/;
}

# Delete obsolete entries from the [sending mail] section
print "# DELETE Mailer\n";
print "# DELETE Method\n";
print "# DELETE Precommand\n";
print "# DELETE Smtp Host\n";
print "# DELETE Smtp Password\n";
print "# DELETE Smtp Port\n";
print "# DELETE Smtp Username\n";

# Write entries to the [Transport 1] section
print "precommand=$data{'Precommand'}\n";
print "port=$data{'Smtp Port'}\n";
if ($data{'Method'} eq "smtp") {
  print "type=smtp\n";
  print "host=$data{'Smtp Host'}\n";
  print "name=$data{'Smtp Host'}\n";
}
else {
  print "type=sendmail\n";
  print "host=$data{'Mailer'}\n";
  print "name=Sendmail\n";
}
print "\n[General]\n";
print "transports=1\n";
