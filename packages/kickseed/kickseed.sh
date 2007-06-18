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

# Save %post script for later execution.
save_post_script () {
	if [ -e "$SPOOL/post.section" ]; then
		mkdir -p "$SPOOL/post"
		i=0
		while [ -e "$SPOOL/post/$i.script" ]; do
			i="$(($i + 1))"
		done
		mv "$SPOOL/post.section" "$SPOOL/post/$i.script"
		if [ "$post_chroot" = 1 ]; then
			touch "$SPOOL/post/$i.chroot"
		else
			rm -f "$SPOOL/post/$i.chroot"
		fi
	fi
}

# Load all handlers.
for handler in "$HANDLERS"/*; do
	. "$handler"
done

# Expects kickstart file as $1.
kickseed () {
	# Parse and execute %pre sections first.
	SECTION=main
	(cat "$1"; echo %final) | while read line; do
		keyword="${line%% *}"
		case $keyword in
			%pre)
				SECTION=pre
				pre_handler_section "${line#* }"
				> "$SPOOL/pre.section"
				continue
				;;
			%packages|%post|%final)
				if [ -e "$SPOOL/pre.section" ]; then
					ks_run_script pre /bin/sh 0 "$SPOOL/pre.section"
					rm -f "$SPOOL/pre.section"
				fi
				SECTION="${keyword#%}"
				continue
				;;
		esac
		if [ "$SECTION" = pre ]; then
			echo "$line" >> "$SPOOL/pre.section"
		fi
	done

	# Parse all other sections.
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
	done < "$1"; echo %final) | while read line; do
		# Work out the section.
		keyword="${line%% *}"
		if [ "$keyword" = '%packages' ]; then
			save_post_script
			SECTION=packages
			continue
		elif [ "$keyword" = '%pre' ]; then
			save_post_script
			SECTION=pre
			continue
		elif [ "$keyword" = '%post' ]; then
			save_post_script
			post_handler_section "${line#* }"
			SECTION=post
			> "$SPOOL/post.section"
			continue
		elif [ "$keyword" = '%final' ]; then
			save_post_script
			for handler in $final_handlers; do
				$handler
			done
			break
		fi

		if [ "$SECTION" = main ]; then
			if [ -z "$keyword" ] || [ "${keyword#\#}" != "$keyword" ]; then
				# Ignore empty lines and comments.
				continue
			fi

			# Delegate to directive handlers.
			if type "${keyword}_handler" >/dev/null 2>&1; then
				eval "${keyword}_handler" "${line#* }"
			else
				die "Unrecognised kickstart command: $keyword"
			fi
		elif [ "$SECTION" = packages ]; then
			if [ -z "$keyword" ] || [ "${keyword#\#}" != "$keyword" ]; then
				# Ignore empty lines and comments.
				continue
			fi

			if [ "$keyword" = '@' ]; then
				die "Package groups not implemented"
			fi

			echo "$line" >> "$SPOOL/$SECTION.section"
		elif [ "$SECTION" = pre ]; then
			# already handled
			continue
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
		ks_preseed base-config base-config/package-selection string \
			"$pattern"
	fi
}

kickseed_post () {
	# TODO: sort numerically
	for script in "$SPOOL/post"/*.script; do
		if [ ! -f "$script" ]; then
			continue
		fi
		CHROOTED=0
		if [ -e "${script%.script}.chroot" ]; then
			CHROOTED=1
		fi
		ks_run_script post /bin/sh "$CHROOTED" "$script"
	done
}
