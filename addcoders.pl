#!/usr/bin/perl

use XML::DOM;

local $/;
my $doc = (new XML::DOM::Parser)->parse(<STDIN>);
my $codermap = $doc->getDocumentElement("codermap");
foreach (@ARGV) {
	my $coder = $doc->createElement("coder");
	$coder->setAttribute("magick", $_);
	$coder->setAttribute("name", "FAIL");
	$codermap->appendChild($coder);
}
$doc->printToFileHandle(\*STDOUT);

