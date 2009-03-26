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
		chmod +x "$POSTSPOOL/$i.script"
		if [ "$post_chroot" = 1 ]; then
			touch "$POSTSPOOL/$i.chroot"
			if [ "$post_interpreter" ]; then
				echo "$post_interpreter" \
					> "$POSTSPOOL/$i.interpreter"
			fi
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
	while read line; do
		line="${line%%}"
		keyword="${line%%[ 	]*}"
		case $keyword in
			%pre)
				SECTION=pre
				pre_handler_section "${line#*[ 	]}"
				> "$SPOOL/parse/pre.section"
				continue
				;;
			%packages|%post)
				SECTION="${keyword#%}"
				continue
				;;
		esac
		if [ "$SECTION" = pre ]; then
			echo "$line" >> "$SPOOL/parse/pre.section"
		fi
	done < "$1"
	if [ -e "$SPOOL/parse/pre.section" ]; then
		chmod +x "$SPOOL/parse/pre.section"
		CODE=0
		ks_run_script pre /bin/sh 0 "$SPOOL/parse/pre.section" || CODE="$?"
		if [ "$CODE" != 0 ]; then
			warn "%pre script exited with error code $CODE"
		fi
		rm -f "$SPOOL/parse/pre.section"
	fi

	# Parse all other sections.
	SECTION=main
	(while read line; do
		line="${line%%}"
		keyword="${line%%[ 	]*}"
		# Deal with %include directives.
		if [ "$keyword" = '%include' ]; then
			rest="${line#*[ 	]}"
			arg="${rest%%[ 	]*}"
			cat "$arg"
		elif [ "$keyword" = '%final' ]; then
			die "%final reserved for internal use"
		else
			echo "$line"
		fi
	done < "$1"; echo %final) | while read line; do
		# Work out the section.
		keyword="${line%%[ 	]*}"
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
			args="${line#*[ 	]}"
			if [ "$args" = "$line" ]; then
				# No arguments.
				args=
			fi
			eval post_handler_section "$args"
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
				group="${line#*[ 	]}"
				# TODO: temporary hack to make at least the
				# standard desktop work
				case $group in
					*\ Standard)
						echo 'task:standard' >> "$SPOOL/parse/$SECTION.section"
						;;
					Ubuntu\ Desktop)
						echo 'task:ubuntu-desktop' >> "$SPOOL/parse/$SECTION.section"
						;;
					Kubuntu\ Desktop)
						echo 'task:kubuntu-desktop' >> "$SPOOL/parse/$SECTION.section"
						;;
					*\ *)
						warn "Package group '$group' not implemented"
						;;
					*)
						# Anything without a space
						# is assumed to be the name
						# of a task; useful for
						# customisers.
						echo "task:$group" >> "$SPOOL/parse/$SECTION.section"
						;;
				esac
			else
				echo "pkg:$line" >> "$SPOOL/parse/$SECTION.section"
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
			case $pkg in
				task:-*|pkg:-*)
					negatives="$negatives $pkg"
					;;
				*)
					positives="$positives $pkg"
					;;
			esac
		done

		# pattern gets: (~nPOS|~nPOS|~nPOS)!~nNEG!~nNEG!~nNEG
		joinpositives=
		for pkg in $positives; do
			case $pkg in
				.)	continue ;;
				task:*)
					element="~t^${pkg#task:}\$"
					tasklist="${tasklist:+$tasklist, }${pkg#task:}"
					;;
				pkg:*)
					element="~n^${pkg#pkg:}\$"
					packagelist="${packagelist:+$packagelist }${pkg#pkg:}"
					;;
			esac
			joinpositives="${joinpositives:+$joinpositives|}$element"
		done
		pattern="($joinpositives)"
		hasnegatives=false
		for pkg in $negatives; do
			case $pkg in
				.)	continue ;;
				task:-*)
					element="~t^${pkg#task:-}\$"
					hasnegatives=:
					;;
				pkg:-*)
					element="~n^${pkg#pkg:-}\$"
					hasnegatives=:
					;;
			esac
			pattern="$pattern!$element"
		done
		# requires pkgsel 0.04ubuntu1; obsolete as of pkgsel
		# 0.07ubuntu1
		ks_preseed d-i pkgsel/install-pattern string "$pattern"
		# requires pkgsel 0.07ubuntu1/0.08
		ks_preseed tasksel tasksel/first multiselect "$tasklist"
		ks_preseed d-i pkgsel/include string "$packagelist"
		if $hasnegatives; then
			warn "exclusions in %packages not supported; remove them manually in %post instead"
		fi
	fi

	# Kickstart installations always run at critical priority.
	ks_preseed d-i debconf/priority 'select' critical
}

kickseed_post () {
	# Post-installation parts of handlers.
	for dir in "$POSTSPOOL"/*.handler; do
		[ -d "$dir" ] || continue
		name="${dir##*/}"
		if type "${name%.handler}_post" >/dev/null 2>&1; then
			ks_run_handler "${name%.handler}_post"
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
		INTERPRETER=/bin/sh
		if [ -e "${script%.script}.interpreter" ]; then
			INTERPRETER="$(cat "${script%.script}.interpreter")"
		fi
		CODE=0
		ks_run_script post "$INTERPRETER" "$CHROOTED" "$script" || CODE="$?"
		if [ "$CODE" != 0 ]; then
			warn "%post script exited with error code $CODE"
		fi
	done
}
