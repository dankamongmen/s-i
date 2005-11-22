#!/usr/bin/perl -w

# Define module to use
use HTML::Parser();

local %tagstatus;
local %example;
local $prevtag='', $titletag;
local $settitle=0;

$example{'print'} = 0;
$example{'in_sect'} = 0;
$example{'first'} = 1;
$example{'new'} = 0;

# Create instance
$p = HTML::Parser->new(
	start_h => [\&start_rtn, 'tagname, text, attr'],
	text_h => [\&text_rtn, 'text'],
	end_h => [\&end_rtn, 'tagname']);
# Start parsing the following HTML string
$p->parse_file('example-preseed-etch-new.xml');
 
# Execute when start tag is encountered
sub start_rtn {
	my ($tagname, $text, $attr) = @_;
	#print "\nStart: $tagname\n";
	#print "Condition: $attr->{condition}\n" if exists $attr->{condition};
	#print "Architecture: $attr->{arch}\n" if exists $attr->{arch};
	if ( $tagname =~ /appendix|sect1|sect2|sect3|para/ ) {
		$tagstatus{$tagname}{'count'} += 1;
		#print "$tagname  $tagstatus{$tagname}{'count'}\n";
	}
	# Assumes that <title> is the first tag after a section tag
	if ( $prevtag =~ /sect1|sect2|sect3/ ) {
		$settitle = ( $tagname eq 'title' );
		$titletag = $prevtag;
		$example{'in_sect'} = 0;
	}
	$prevtag = $tagname;
	if ( $tagname eq 'informalexample' ) {
		$example{'print'} = 1;
		$example{'new'} = 1;
	}
}
 
# Execute when text is encountered
sub text_rtn {
my ($text) = @_;
	if ( $settitle ) {
		$tagstatus{$titletag}{'title'} = $text;
		$settitle = 0;
	}
	if ( $example{'print'} ) {
		# Print section headers
		for ($s=1; $s<=3; $s++) {
			my $sect="sect$s";
			if ( $tagstatus{$sect}{'title'} ) {
				print "\n" if ( $s == 1 && ! $example{'first'} );
				for ( $i = 1; $i <= 5 - $s; $i++ ) { print "#"; };
				print " $tagstatus{$sect}{'title'}\n";
				delete $tagstatus{$sect}{'title'};
			}
		}

		# Clean leading whitespace
		if ( $example{'new'} ) {
			$text =~ s/^[[:space:]]*//;
		}

		print "$text";

		$example{'first'} = 0;
		$example{'new'} = 0;
		$example{'in_sect'} = 1;
	}
}
 
# Execute when the end tag is encountered
sub end_rtn {
	my ($tagname) = @_;
	#print "\nEnd: $tagname\n";
	if ( $tagname eq 'informalexample' ) {
		$example{'print'} = 0;
	}
	if ( $tagname =~ /appendix|sect1|sect2|sect3|para/ ) {
		delete $tagstatus{$tagname}{'title'} if exists $tagstatus{$tagname}{'title'};
		if ( $example{'in_sect'} ) {
			print "\n";
			$example{'in_sect'} = 0;
		}
		$tagstatus{$tagname}{'count'} -= 1;
		#print "$tagname  $tagstatus{$tagname}{'count'}\n";
	}
}
