 use MIDI; # uses MIDI::Opus et al
 foreach $one (@ARGV) {
   my $opus = MIDI::Opus->new({ 'from_file' => $one, 'no_parse' => 0 });
   print "$one has ", scalar( $opus->tracks ), " tracks\n";
   
   my @tracks = $opus->tracks;
   my @events = $tracks[2]->events;
   foreach my $event (@events) {
      print "[",join (',', @{$event || []}), "]","\n";
   }
   #use Data::Dumper;
   #print STDERR Dumper($tracks[0]);
 }
 exit;
