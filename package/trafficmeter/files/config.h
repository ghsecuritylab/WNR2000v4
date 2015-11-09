#ifndef __CONFIG_H__
#define __CONFIG_H__

/* which mtd used record traffic meter data */
#define TRAFFIC_METER_MTD "/dev/mtd7"

/* WAN interface name of router */
#define WAN_DHCP_IFNAME "br1"
/* PPP interface name of router */
#define WAN_PPP_IFNAME "ppp0"
#define WAN_ETH_STATUS "/tmp/port_status"
#define WAN_PPP_STATUS "/tmp/ppp/ppp0-status"
#define AUTO_DISABLE
#define UPDATE_STATISTICS
#define ACCURATE_CONTROL
/* output files, like as status, statistics, date and volume left, warning messages,
 *    and config file of traffic meter. */
#define TMP_TRAFFIC_WARNINGMSG "/tmp/traffic_meter/traffic_meter_warning_message"
#define TMP_TRAFFIC_CONFIG "/tmp/traffic_meter/traffic_meter.conf"
#define TMP_TRAFFIC_LEFT "/tmp/traffic_meter/date_and_volume_left"
#define TMP_TRAFFIC_STATISTICS "/tmp/traffic_meter/traffic_statistics"
#define TMP_TRAFFIC_STATUS "/tmp/traffic_meter/traffic_status"

/* support new feature of NTP described in traffic meter spec 1.9 */
#define NTP_FEATURE

#endif
