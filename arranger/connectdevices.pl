#!/usr/bin/perl 

use warnings;
use strict;

sub search_device 
{
	my $what = shift;
	my $list = shift;
	my $first = '';
	for(@$list)
	{
		if($_ =~ m/^client\s(\d+)(.*)/) {
			$first = $1;
		}
		elsif($_ =~ m/\s+(\d+)\s\'$what\s*\'/) {
			return "$first:$1";
		}
	}
	return '';
}

my @devices = @ARGV;




my @listdevices = `aconnect -li`;
my @listdevices_in = `aconnect -lo`;

my $output_id = search_device('Virtual RawMIDI',\@listdevices);

if($output_id) {
	for(@devices)
	{
		my $input_id = search_device($_,\@listdevices_in);
		if($input_id) {
			system("aconnect $output_id $input_id");
		}
	}
}
