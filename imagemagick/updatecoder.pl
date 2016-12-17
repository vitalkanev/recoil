#!/usr/bin/perl
use XML::DOM;

# load coder.xml
local $/;
my $doc = (new XML::DOM::Parser)->parse(<STDIN>);
my $codermap = $doc->getDocumentElement();

# remove all RECOIL entries
for my $coder ($codermap->getChildNodes()) {
	if ($coder->getNodeType() == ELEMENT_NODE && $coder->getTagName() eq 'coder' && $coder->getAttribute('name') eq 'RECOIL') {
		$codermap->removeChild($coder);
	}
}

# optionally add RECOIL entries
if (@ARGV) {
	# get unique extensions from formats.xml
	my @exts = (new XML::DOM::Parser)->parsefile($ARGV[0])->getElementsByTagName('ext');
	my %exts = map { $_->getFirstChild()->getData() => 1 } @exts;
	@exts = sort keys %exts;
	# add a RECOIL entry for each extension
	for (@exts) {
		my $coder = $doc->createElement('coder');
		$coder->setAttribute('magick', $_);
		$coder->setAttribute('name', 'RECOIL');
		$codermap->appendChild($coder);
	}
}

# write coder.xml
$doc->printToFileHandle(\*STDOUT);
