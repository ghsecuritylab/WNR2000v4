#!/bin/sh

uhttpd_stop()
{
	killall uhttpd
}

uhttpd_start()
{
	/sbin/artmtd -r region
	uhttpd -e "/usr/sbin/detwan"
}

case "$1" in
	stop)
		uhttpd_stop
	;;
	start)
		uhttpd_start
	;;
	restart)
		uhttpd_stop
		uhttpd_start
	;;
	*)
		logger -- "usage: $0 start|stop|restart"	
	;;
esac

