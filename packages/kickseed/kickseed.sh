#! /bin/sh
# Caller should set -e.

if [ -z "$HANDLERS" ]; then
	HANDLERS=/lib/kickseed/handlers
fi
if [ -z "$SPOOL" ]; then
	SPOOL=/var/spool/kickseed
fi
if [ -z "$POSTSPOOL" ]; then
	POSTSPOOL="$SPOOL/parse/post"
fi
if [ -z "$SUPPORTEDLOCALES" ]; then
	SUPPORTEDLOCALES=/etc/SUPPORTED-short
fi

warn () {
	ks_log "$@"
}

die () {
	warn "$@"
	exit 1
}

warn_getopt () {
	warn "Failed to parse $1 options"
}

warn_bad_opt () {
	warn "Unimplemented $1 option $2"
}

warn_bad_arg () {
	warn "Unimplemented $1 $2 argument $3"
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
	if [ -e "$SPOOL/parse/post.section" ]; then
		mkdir -p "$POSTSPOOL"
		i=0
		while [ -e "$POSTSPOOL/$i.script" ]; do
			i="$(($i + 1))"
		done
		mv "$SPOOL/parse/post.section" "$POSTSPOOL/$i.script"
		if [ "$post_chroot" = 1 ]; then
			touch "$POSTSPOOL/$i.chroot"
		else
			rm -f "$POSTSPOOL/$i.chroot"
		fi
	fi
}

# Load all handlers.
for handler in "$HANDLERS"/*; do
	. "$handler"
done

# Expects kickstart file as $1.
kickseed () {
	rm -rf "$SPOOL/parse"
	mkdir -p "$SPOOL/parse"

	# Parse and execute %pre sections first.
	SECTION=main
	(cat "$1"; echo %final) | while read line; do
		keyword="${line%% *}"
		case $keyword in
			%pre)
				SECTION=pre
				pre_handler_section "${line#* }"
				> "$SPOOL/parse/pre.section"
				continue
				;;
			%packages|%post|%final)
				if [ -e "$SPOOL/parse/pre.section" ]; then
					if ! ks_run_script pre /bin/sh 0 "$SPOOL/parse/pre.section"; then
						warn "%pre script exited with error code $?"
					fi
					rm -f "$SPOOL/parse/pre.section"
				fi
				SECTION="${keyword#%}"
				continue
				;;
		esac
		if [ "$SECTION" = pre ]; then
			echo "$line" >> "$SPOOL/parse/pre.section"
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
			> "$SPOOL/parse/post.section"
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
				args="${line#*[ 	]}"
				if [ "$args" = "$line" ]; then
					# No arguments.
					args=
				fi
				# This gets ...='\$foo' wrong, but it's
				# better than the alternative (broken
				# crypted passwords) for now.
				args="$(printf %s "$args" | sed 's/\$/\\$/g')"
				eval "${keyword}_handler" "$args"
			else
				warn "Unrecognised kickstart command: $keyword"
			fi
		elif [ "$SECTION" = packages ]; then
			if [ -z "$keyword" ] || [ "${keyword#\#}" != "$keyword" ]; then
				# Ignore empty lines and comments.
				continue
			fi

			if [ "$keyword" = '@' ]; then
				group="${line#* }"
				# TODO: temporary hack to make at least the
				# standard desktop work
				case $group in
					Ubuntu\ Desktop)
						echo "~tubuntu-desktop" >> "$SPOOL/parse/$SECTION.section"
						;;
					Kubuntu\ Desktop)
						echo "~tkubuntu-desktop" >> "$SPOOL/parse/$SECTION.section"
						;;
					*\ *)
						warn "Package group '$group' not implemented"
						;;
					*)
						# Anything without a space
						# is assumed to be the name
						# of a task; useful for
						# customisers.
						echo "~t$group" >> "$SPOOL/parse/$SECTION.section"
						;;
				esac
			else
				echo "$line" >> "$SPOOL/parse/$SECTION.section"
			fi
		elif [ "$SECTION" = pre ]; then
			# already handled
			continue
		else
			echo "$line" >> "$SPOOL/parse/$SECTION.section"
		fi
	done || exit $?

	if [ -s "$SPOOL/parse/packages.section" ]; then
		# Handle %packages.
		# TODO: doesn't allow removal of packages from ubuntu-base

		packages="$(cat "$SPOOL/parse/packages.section")"

		positives=.
		negatives=.
		for pkg in $packages; do
			if [ "${pkg#-}" != "$pkg" ]; then
				negatives="$negatives ${pkg#-}"
			else
				positives="$positives $pkg"
			fi
		done

		# pattern gets: (~nPOS|~nPOS|~nPOS)!~nNEG!~nNEG!~nNEG
		joinpositives=
		for pkg in $positives; do
			case $pkg in
				.)	continue ;;
				~t*)	element="$pkg" ;;
				*)	element="~n$pkg" ;;
			esac
			joinpositives="${joinpositives:+$joinpositives|}$element"
		done
		pattern="($joinpositives)"
		for pkg in $negatives; do
			case $pkg in
				.)	continue ;;
				~t*)	element="$pkg" ;;
				*)	element="~n$pkg" ;;
			esac
			pattern="$pattern!$element"
		done
		# introduced in base-config 2.61ubuntu2; Debian would need
		# tasksel preseeding instead
		ks_preseed base-config base-config/package-selection string \
			"$pattern"
	fi

	# Kickstart installations always run at critical priority.
	ks_preseed d-i debconf/priority 'select' critical
	# TODO: not in Debian
	ks_preseed base-config base-config/priority 'select' critical
}

kickseed_post () {
	# Post-installation parts of handlers.
	for dir in "$POSTSPOOL"/*.handler; do
		[ -d "$dir" ] || continue
		name="${dir##*/}"
		if type "${name%.handler}_post" >/dev/null 2>&1; then
			eval "${name%.handler}_post"
		else
			warn "Missing post-installation handler: $name"
		fi
	done

	# User-supplied post-installation scripts.
	# TODO: sort numerically
	for script in "$POSTSPOOL"/*.script; do
		[ -f "$script" ] || continue
		CHROOTED=0
		if [ -e "${script%.script}.chroot" ]; then
			CHROOTED=1
		fi
		if ! ks_run_script post /bin/sh "$CHROOTED" "$script"; then
			warn "%post script exited with error code $?"
		fi
	done
}
