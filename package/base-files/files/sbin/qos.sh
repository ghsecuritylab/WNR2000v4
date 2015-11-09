#!/bin/sh
TC="/usr/sbin/tc"
IPTABLES="/usr/sbin/iptables"
NVRAM="/bin/config"
ECHO="/bin/echo"
WAN_IF="$($NVRAM get wan_ifnames)"
WAN_PROTO="$($NVRAM get wan_proto)"
FILTER_ADD="$TC filter add dev $WAN_IF"
UPRATE="$($NVRAM get qos_uprate)"
QoS_ENABLE="$($NVRAM get qos_endis_on)"
BANDCTL="$($NVRAM get qos_threshold)"
WAN_SPEED=`cat /tmp/WAN_status | cut -f 1 -d 'M'`


start(){
	if [ "x$QoS_ENABLE" != "x1" ]; then
		$ECHO -n 0:$BANDCTL > /proc/MFS
		return
	fi

	if [ "x$WAN_PROTO" = "xpptp" ]; then
		if [ "x$BANDCTL" = "x0" ] || [ $UPRATE -le 0 ] || [ $UPRATE -gt 1000000 ]; then
			UPRATE=1000000
		fi
	elif [ "x$WAN_PROTO" = "xpppoe" ]; then
		if [ "x$BANDCTL" = "x0" ] || [ $UPRATE -le 0 ] || [ $UPRATE -gt 1000000 ]; then
			UPRATE=1000000
		fi
	else
		if [ "x$BANDCTL" = "x0" ] || [ $UPRATE -le 0 ] || [ $UPRATE -gt 1000000 ]; then
 			UPRATE=1000000
 		fi
	fi

	$ECHO -n $UPRATE:$BANDCTL > /proc/MFS

}

stop(){
	$ECHO -n 0:$BANDCTL > /proc/MFS
}

status(){
	$IPTABLES -t mangle -nvL
}
								 
case "$1" in
	stop)
	stop
	;;
	start | restart )
	stop
	start
	;;
	status)
	status
	;;
	*)
	echo $"Usage:$0 {start|stop|restart|status}"
	exit 1
esac

