use MIDI;
use MIDI::Opus;
use MIDI::Score;
use MIDI::Simple;
use Data::Dumper;

new_score;

my $opus = MIDI::Opus->new({'from_file' => $ARGV[0]});

my $count=0;
my $mend_time = 0;
my @track_scores;

foreach my $track ($opus->tracks) {
	my ($score_r, $end_time) = MIDI::Score::events_r_to_score_r($track->events_r);
	if ($end_time > $max_end_time) {
		$max_end_time = $end_time;
	}

	$track_scores[$count] = $score_r;
	
#	print Dumper(@{$score_r});
	$count++;
}

my @merged;

my $finished = 0;
while(!$finished) {
	$finished = 1;
	my $score_id = 0;
	my $to_pop = -1;
	foreach my $part (@track_scores) {
		if (scalar @{$part}) {
			$finished = 0;
			if ($to_pop == -1) {
				$to_pop = $score_id;
			}
			elsif( $part->[0][1] < $track_scores[$to_pop]->[0][1]) {
				$to_pop = $score_id;
			}
		}

		$score_id++;
	}

	if ($to_pop != -1) {
		my $part = $track_scores[$to_pop];
		my $first = shift @{$part};
		push @merged, $first;
	}
	else {
		$finished = 1;
	}
}

my @filtered;

my %bass_channels;

foreach my $item (@merged) {
	next unless $item->[0] eq 'patch_change';
	my $patch = $item->[3];
	my $channel = $item->[2];
	if ($patch >= 33 && $patch <= 40 ) {
		$bass_channels{$channel} = 1;
	}
}

foreach my $item (@merged) {
	my $channel = -1;
	my $spos = -1;
	if ($item->[0] eq 'patch_change') {	
		$channel = $item->[2];
		$spos = 2;
	}
	elsif ($item->[0] eq 'key_after_touch') {
		$channel = $item->[2];
		$spos = 2;
	}
	elsif ($item->[0] eq 'channel_after_touch') {
		$channel = $item->[2];
		$spos = 2;
	}
	elsif ($item->[0] eq 'control_change') {
		$channel = $item->[2];
		$spos = 2;
	}
	elsif ($item->[0] eq 'pitch_wheel_change') {
		$channel = $item->[2];
		$spos = 2;
	}
	elsif ($item->[0] eq 'note') {
		$channel = $item->[3];
		$spos = 3;
	}

	if ($channel == -1 || $channel == 9) {
		push @filtered, $item;
	}

	if ($bass_channels{$channel}) {
		$item->[$spos] = 0;
		push @filtered, $item;
	}
}



$Tempo = $opus->ticks;
@Score =  @filtered;
$Time = $max_end_time;

my $outname = $ARGV[1] || 'merged.mid';

write_score($outname);
