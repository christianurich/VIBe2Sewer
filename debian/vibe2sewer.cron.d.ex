#
# Regular cron jobs for the vibe2sewer package
#
0 4	* * *	root	[ -x /usr/bin/vibe2sewer_maintenance ] && /usr/bin/vibe2sewer_maintenance
