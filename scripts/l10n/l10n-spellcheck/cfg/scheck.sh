#!/bin/sh
export PATH=${HOME}/l10n-spellcheck:$PATH

case "$1" in
man)
	l10n-spellcheck.sh ~/l10n-spellcheck/cfg/manual_d-i
	;;
l1-ps)
	l10n-spellcheck.sh ~/l10n-spellcheck/cfg/level1-post-sarge
	;;
l1)
	l10n-spellcheck.sh ~/l10n-spellcheck/cfg/level1
	;;
l2)
	l10n-spellcheck.sh ~/l10n-spellcheck/cfg/level2
	;;
l3)
	l10n-spellcheck.sh ~/l10n-spellcheck/cfg/level3
	;;
l4)
	l10n-spellcheck.sh ~/l10n-spellcheck/cfg/level4
	;;
l5)
	l10n-spellcheck.sh ~/l10n-spellcheck/cfg/level5
	;;
esac
