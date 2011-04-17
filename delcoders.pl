#!/usr/bin/perl

use XML::DOM;

local $/;
my $doc = (new XML::DOM::Parser)->parse(<STDIN>);
my $codermap = $doc->getDocumentElement("codermap");
for my $coder ($codermap->getChildNodes()) {
	if ($coder->getNodeType == ELEMENT_NODE && $coder->getTagName eq "coder" && $coder->getAttribute("name") eq "FAIL") {
		$codermap->removeChild($coder);
	}
}
$doc->printToFileHandle(\*STDOUT);

