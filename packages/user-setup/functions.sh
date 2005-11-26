# Returns a true value if there seems to be a system user account.
is_system_user () {
	if ! [ -e $ROOT/etc/passwd ]; then
		return 1
	fi

        # Assume NIS, or any uid from 1000 to 29999,  means there is a user.
        if $chroot $ROOT grep -q '^+:' /etc/passwd || \
           $chroot $ROOT grep -q '^[^:]*:[^:]*:[1-9][0-9][0-9][0-9]:' /etc/passwd || \
           $chroot $ROOT grep -q '^[^:]*:[^:]*:[12][0-9][0-9][0-9][0-9]:' /etc/passwd; then
                return 0
        else
                return 1
        fi
}

# Returns a true value if root already has a password.
root_password () {
	# Assume there is a root password if NIS is being used.
	if $chroot $ROOT grep -q '^+:' /etc/passwd; then
		return 0
	fi

	if [ -e $ROOT/etc/shadow ] && \
	   [ "`$chroot $ROOT grep ^root: /etc/shadow | cut -d : -f 2`" -a \
	     "`$chroot $ROOT grep ^root: /etc/shadow | cut -d : -f 2`" != '*' ]; then
		return 0
	fi
	
	if [ -e $ROOT/etc/passwd ] && \
		[ "`$chroot $ROOT grep ^root: /etc/passwd | cut -d : -f 2`" ] && \
		[ "`$chroot $ROOT grep ^root: /etc/passwd | cut -d : -f 2`" != 'x' ]; then
			return 0
	fi

	return 1
}
