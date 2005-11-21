#!/usr/bin/perl -w

# Define module to use
 use HTML::Parser();

local %tagstatus;
local $lasttag='', $titletag;
local $settitle=0, $printexample=0, $exampleinsect=0, $newexample;
local $BOF=1;

 # Create instance
 $p = HTML::Parser->new(
                 start_h => [\&start_rtn, 'tagname, text, attr'],
                 text_h => [\&text_rtn, 'text'],
                 end_h => [\&end_rtn, 'tagname']);
 # Start parsing the following HTML string
 #$p->report_tags( qw(appendix sect1 sect2 sect3 para informalexample title) );
 $p->parse_file('example-preseed-etch-new.xml');
 
 sub start_rtn {
 # Execute when start tag is encountered
     my ($tagname, $text, $attr) = @_;
     #print "\nStart: $tagname\n";
     #print "Condition: $attr->{condition}\n" if exists $attr->{condition};
     #print "Id: $attr->{id}\n" if exists $attr->{id};
     if ( $tagname =~ /appendix|sect1|sect2|sect3|para/ ) {
     	$tagstatus{$tagname}{'count'} += 1;
	#print "$tagname  $tagstatus{$tagname}{'count'}\n";
     }
     if ( $lasttag =~ /sect1|sect2|sect3/ ) {
     	$settitle = ( $tagname eq 'title' );
     	$titletag = $lasttag;
	$exampleinsect=0;
     }
     $lasttag = $tagname;
     if ( $tagname eq 'informalexample' ) {
     	$printexample = 1;
	$newexample = 1;
     }
 }
 
 sub text_rtn {
 # Execute when text is encountered
     my ($text) = @_;
     if ( $settitle ) {
     	$tagstatus{$titletag}{'title'} = $text;
	$settitle = 0;
     }
     if ( $printexample ) {
        for ($s=1; $s<=3; $s++) {
		my $sect="sect$s";
		if ( $tagstatus{$sect}{'title'} ) {
			print "\n" if ($s == 1 && ! $BOF);
			for ($i=1; $i<=5-$s; $i++) { print "#"; };
			print " $tagstatus{$sect}{'title'}\n";
			delete $tagstatus{$sect}{'title'};
		}
	}
	if ( $newexample ) {
		$text =~ s/^[[:space:]]*//;
	}
	#$text =~ s/[[:space:]]*$//;
     	print "$text";
	$newexample=0; $exampleinsect=1;
	$BOF=0;
     }
 }
 
 sub end_rtn {
 # Execute when the end tag is encountered
     my ($tagname) = @_;
     #print "\nEnd: $tagname\n";
     if ( $tagname =~ /appendix|sect1|sect2|sect3|para/ ) {
     	$tagstatus{$tagname}{'count'} -= 1;
	#print "$tagname  $tagstatus{$tagname}{'count'}\n";
	#print "Title: $tagstatus{$tagname}{'title'}\n" if $tagstatus{$tagname}{'title'};
	if ( $exampleinsect ) {
     		print "\n";
		$exampleinsect=0;
	}
     }
     if ( $tagname eq 'informalexample' ) {
     	$printexample = 0;
     }
 }
