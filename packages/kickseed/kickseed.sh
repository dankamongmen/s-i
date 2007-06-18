#! /bin/sh
# Caller should set -e.

die () {
	echo "$@" >&2
	exit 1
}

die_getopt () {
	die "Failed to parse $1 options"
}

die_bad_opt () {
	die "Unimplemented $1 option $2"
}

die_bad_arg () {
	die "Unimplemented $1 $2 argument $3"
}

# Allow handler files to register functions to be called at the end of
# processing; this lets them build up preseed answers over several handler
# calls.
final_handlers=
register_final () {
	final_handlers="$final_handlers $1"
}

# Load all handlers.
for handler in "$HANDLERS"/*; do
	. "$handler"
done

# Expects kickstart file on stdin.
kickseed () {
	SECTION=main

	(while read line; do
		keyword="${line%% *}"
		# Deal with %include directives.
		if [ "$keyword" = '%include' ]; then
			rest="${line#* }"
			arg="${rest%% *}"
			cat "$arg"
		elif [ "$keyword" = '%final' ]; then
			die "%final reserved for internal use"
		else
			echo "$line"
		fi
	done; echo %final) | while read line; do
		# Work out the section.
		keyword="${line%% *}"
		if [ -z "$keyword" ] || [ "${keyword#\#}" != "$keyword" ]; then
			# Ignore empty lines and comments.
			continue
		elif [ "$keyword" = '%packages' ]; then
			SECTION=packages
			continue
		elif [ "$keyword" = '%pre' ]; then
			SECTION=pre
			continue
		elif [ "$keyword" = '%post' ]; then
			SECTION=post
			continue
		elif [ "$keyword" = '%final' ]; then
			for handler in $final_handlers; do
				$handler
			done
			break
		fi

		if [ "$SECTION" = main ]; then
			# Delegate to directive handlers.
			if type "${keyword}_handler" >/dev/null 2>&1; then
				OPTIND=1
				"${keyword}_handler" ${line#* }
			else
				die "Unrecognised kickstart command: $keyword"
			fi
		elif [ "$SECTION" = packages ] && [ "$keyword" = '@' ]; then
			die "Package groups not implemented"
		else
			echo "$line" >> "$SPOOL/$SECTION.section"
		fi
	done || exit $?

	if [ -s "$SPOOL/packages.section" ]; then
		# Handle %packages.
		# TODO: doesn't allow removal of packages from ubuntu-base

		packages="$(< "$SPOOL/packages.section")"

		positives=.
		negatives=.
		for pkg in $packages; do
			if [ "${pkg#-}" != "$pkg" ]; then
				negatives="$negatives ${pkg#-}"
			else
				positives="$positives $pkg"
			fi
		done

		# pattern gets: (pos|pos|pos)!neg!neg!neg
		joinpositives=
		for pkg in $positives; do
			if [ "$pkg" = . ]; then
				continue
			fi
			joinpositives="${joinpositives:+$joinpositives|}$pkg"
		done
		pattern="($joinpositives)"
		for pkg in $negatives; do
			if [ "$pkg" = . ]; then
				continue
			fi
			pattern="$pattern!$pkg"
		done
		# introduced in base-config 2.61ubuntu2; Debian would need
		# tasksel preseeding instead
		preseed base-config base-config/package-selection string \
			"$pattern"
	fi
}
