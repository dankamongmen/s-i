#! /bin/sh

xconfig_handler () {
	eval set -- "$(getopt -o '' -l noprobe,card:,videoram:,monitor:,hsync:,vsync:,defaultdesktop:,startxonboot,resolution:,depth: -- "$@")" || die_getopt xconfig
	while :; do
		case $1 in
			--noprobe)
				# TODO: xresprobe code doesn't honour this
				preseed xserver-xorg xserver-xorg/autodetect_monitor boolean false
				shift
				;;
			--card)
				# TODO: translate card name
				preseed xserver-xorg xserver-xorg/config/device/driver 'select' "$2"
				shift 2
				;;
			--videoram)
				preseed xserver-xorg xserver-xorg/config/device/video_ram string "$2"
				shift 2
				;;
			--monitor)
				# TODO: no monitor database; we could
				# preseed the identifier but there's little
				# point
				die_bad_opt xconfig monitor
				;;
			--hsync)
				preseed xserver-xorg xserver-xorg/config/monitor/horiz-sync string "$2"
				shift 2
				;;
			--vsync)
				preseed xserver-xorg xserver-xorg/config/monitor/vert-refresh string "$2"
				shift 2
				;;
			--defaultdesktop)
				if [ "$2" = GNOME ]; then
					: # Ubuntu default
				else
					die_bad_arg xconfig defaultdesktop "$2"
				fi
				shift 2
				;;
			--startxonboot)
				# TODO: this is true by default already
				preseed base-config base-config/start-display-manager boolean true
				shift
				;;
			--resolution)
				# TODO: ideally we'd just pass the
				# resolution and let X work out the best
				# refresh rate
				preseed xserver-xorg xserver-xorg/config/monitor/mode-list 'select' "$2 @ 60Hz"
				shift 2
				;;
			--depth)
				if [ "$2" = 32 ]; then
					depth=24
				else
					depth="$2"
				fi
				preseed xserver-xorg xserver-xorg/config/display/default_depth 'select' "$depth"
				shift 2
				;;
			--)	shift; break ;;
			*)	die_getopt xconfig ;;
		esac
	done
}
