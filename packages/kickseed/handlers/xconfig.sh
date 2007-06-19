#! /bin/sh

xconfig_handler () {
	eval set -- "$(getopt -o '' -l noprobe,card:,videoram:,monitor:,hsync:,vsync:,defaultdesktop:,startxonboot,resolution:,depth: -- "$@")" || { warn_getopt xconfig; return; }
	while :; do
		case $1 in
			--noprobe)
				# TODO: xresprobe code doesn't honour this
				ks_preseed xserver-xorg xserver-xorg/autodetect_monitor boolean false
				shift
				;;
			--card)
				# TODO: translate card name
				ks_preseed xserver-xorg xserver-xorg/config/device/driver 'select' "$2"
				shift 2
				;;
			--videoram)
				ks_preseed xserver-xorg xserver-xorg/config/device/video_ram string "$2"
				shift 2
				;;
			--monitor)
				# TODO: no monitor database; we could
				# preseed the identifier but there's little
				# point
				warn_bad_opt xconfig monitor
				shift 2
				;;
			--hsync)
				ks_preseed xserver-xorg xserver-xorg/config/monitor/horiz-sync string "$2"
				shift 2
				;;
			--vsync)
				ks_preseed xserver-xorg xserver-xorg/config/monitor/vert-refresh string "$2"
				shift 2
				;;
			--defaultdesktop)
				if [ "$2" = GNOME ]; then
					: # Ubuntu default
				else
					warn_bad_arg xconfig defaultdesktop "$2"
				fi
				shift 2
				;;
			--startxonboot)
				# TODO: this is true by default already
				shift
				;;
			--resolution)
				case $2 in
					640x480)
						modes='640x480'
						;;
					800x600)
						modes='800x600, 640x480'
						;;
					1024x768)
						modes='1024x768, 800x600, 640x480'
						;;
					*)
						modes="$2, 1024x768, 800x600, 640x480"
						;;
				esac
				ks_preseed xserver-xorg xserver-xorg/config/display/modes multiselect "$modes"
				shift 2
				;;
			--depth)
				if [ "$2" = 32 ]; then
					depth=24
				else
					depth="$2"
				fi
				ks_preseed xserver-xorg xserver-xorg/config/display/default_depth 'select' "$depth"
				shift 2
				;;
			--)	shift; break ;;
			*)	warn_getopt xconfig; return ;;
		esac
	done
}
