/*
 *	Handle incoming frames
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/netfilter_bridge.h>
#include "br_private.h"
#ifdef CONFIG_DNI_MCAST_TO_UNICAST
#include <linux/ip.h>
#include <linux/igmp.h>

#ifdef CONFIG_BR_TM_ACCURATE_CONTROL
unsigned long long  tmp_counter;
#endif

static inline void 
add_mac_cache(struct sk_buff *skb)
{
      unsigned char i, num = 0xff;
      unsigned char *src, check = 1;
      struct iphdr *iph;
      struct ethhdr *ethernet=(struct ethhdr *)skb->mac_header;

      iph = (struct iphdr *)skb->network_header;
      src = ethernet->h_source;

      for (i = 0; i < MCAST_ENTRY_SIZE; i++)
      {
              if (mac_cache[i].valid)
                      if ((++mac_cache[i].count) == MAX_CLEAN_COUNT)
                              mac_cache[i].valid = 0;
      }

      for (i = 0; i < MCAST_ENTRY_SIZE; i++)
      {
              if (mac_cache[i].valid)
              {
                      if (mac_cache[i].sip==iph->saddr)
                      {
                              num = i;
                              break;
                      }
              }
              else if (check)
              {
                      num=i;
                      check = 0;
              }
      }

      if (num < MCAST_ENTRY_SIZE)
      {
              mac_cache[num].valid = mac_cache[num].count = 1;
              memcpy(mac_cache[num].mac, src, 6);
              mac_cache[num].sip = iph->saddr;
              mac_cache[num].dev = skb->dev;
      }
}

#endif
/* Bridge group multicast address 802.1d (pg 51). */
const u8 br_group_address[ETH_ALEN] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00 };

static void br_pass_frame_up(struct net_bridge *br, struct sk_buff *skb)
{
	struct net_device *indev, *brdev = br->dev;
#ifdef CONFIG_BR_TM_ACCURATE_CONTROL
        char str1[10] = "br1";
        struct sk_buff *skb_send = NULL;
        struct nlmsghdr * nlh = NULL;
        struct msg_data sendata;
        int nt_ret = 0;
#endif
#ifdef CONFIG_DNI_MCAST_TO_UNICAST
      unsigned char *dest;
      struct iphdr *iph;
      unsigned char proto=0;
      struct ethhdr *ethernet=(struct ethhdr *)skb->mac_header;

      // if skb come from wireless interface, ex. ath0, ath1, ath2...
      if (skb->dev->name[0] == 'a')
      {
              iph = (struct iphdr *)skb->network_header;
              proto =  iph->protocol;
              dest = ethernet->h_dest;
              if ( igmp_snoop_enable && MULTICAST_MAC(dest) 
                       && (ethernet->h_proto == ETH_P_IP))
              {
                      if (proto == IPPROTO_IGMP)
                              add_mac_cache(skb);
              }
      }
#endif
#ifdef CONFIG_BR_TM_ACCURATE_CONTROL 
	if(need_drop == 1 && strcmp(br->dev->name,str1) == 0){
		kfree_skb(skb);	
		return ;
	}
#endif

#ifdef CONFIG_BRIDGE_NETGEAR_ACL
	if (!br_acl_should_pass(br, skb, ACL_CHECK_SRC)) {
		br->dev->stats.rx_dropped++;
		kfree_skb(skb);
		return;
	}
#endif

	brdev->stats.rx_packets++;
	brdev->stats.rx_bytes += skb->len;
#ifdef CONFIG_BR_TM_ACCURATE_CONTROL

        if(tm_limit != 0xffffffffffffffff){

               if(strcmp(br->dev->name,str1) == 0)
                       counter_rx += skb->len;


               if(br_tm_dir == 1)
                        tmp_counter = counter_rx + counter_tx;
               else
                        tmp_counter = counter_rx;

               if( tmp_counter >= tm_limit && msg_recive == 1){
                sendata.backpid = 0;
                sendata.leftdata = 0;
                sendata.tm_dir = 0;
                msg_recive = 0;

                skb_send = alloc_skb(NLMSG_SPACE(1024), GFP_KERNEL);
                if (skb_send == NULL) {
                        printk("alloc skb failed\n");
                        return;
                }

                NETLINK_CB(skb_send).pid = 0;
                NETLINK_CB(skb_send).dst_group = 1;
                
                nlh = NLMSG_PUT(skb_send, 0, 0, 0, 1024);
                memcpy(NLMSG_DATA(nlh),&sendata,sizeof(sendata));

                if(br_tm_dir != 2){
                        nt_ret = netlink_broadcast(nl_sk, skb_send, 0, 1, GFP_KERNEL);
			need_drop = 1;
			printk("Need Drop ...............\n");
                        if(nt_ret < 0)
                                 printk("KERNEL broadcast failed  %d ...\n",nt_ret);
                }

                need_back = 0;
                counter_rx = 0;
                counter_tx = 0;
                tmp_counter = 0;
                tm_limit = 0xffffffffffffffff;

        nlmsg_failure:
                counter_tx =0;
                tmp_counter = 0;
                counter_rx = 0;
                tm_limit = 0xffffffffffffffff;
                }
       }
#endif

	indev = skb->dev;
	skb->dev = brdev;

	NF_HOOK(PF_BRIDGE, NF_BR_LOCAL_IN, skb, indev, NULL,
		netif_receive_skb);
}

/* note: already called with rcu_read_lock (preempt_disabled) */
int br_handle_frame_finish(struct sk_buff *skb)
{
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	struct net_bridge_port *p = rcu_dereference(skb->dev->br_port);
	struct net_bridge *br;
	struct net_bridge_fdb_entry *dst;
	struct sk_buff *skb2;

	if (!p || p->state == BR_STATE_DISABLED)
		goto drop;

	/* insert into forwarding database after filtering to avoid spoofing */
	br = p->br;
	br_fdb_update(br, p, eth_hdr(skb)->h_source);

	if (p->state == BR_STATE_LEARNING)
		goto drop;

	/* The packet skb2 goes to the local host (NULL to skip). */
	skb2 = NULL;

	if (br->dev->flags & IFF_PROMISC)
		skb2 = skb;

	dst = NULL;

	if (is_multicast_ether_addr(dest) 
#ifdef CONFIG_DNI_IPV6_PASSTHROUGH
		|| (skb->protocol == __constant_htons(ETH_P_IPV6))
#endif
	) {
		br->dev->stats.multicast++;
		skb2 = skb;
	} else if ((dst = __br_fdb_get(br, dest)) && dst->is_local) {
		skb2 = skb;
		/* Do not forward the packet since it's local. */
		skb = NULL;
	}

	if (skb2 == skb)
		skb2 = skb_clone(skb, GFP_ATOMIC);

	if (skb2)
		br_pass_frame_up(br, skb2);

	if (skb) {
		if (dst)
			br_forward(dst->dst, skb);
		else {
			br_flood_forward(br, skb);			
		}
	}

out:
	return 0;
drop:
	kfree_skb(skb);
	goto out;
}

/* note: already called with rcu_read_lock (preempt_disabled) */
static int br_handle_local_finish(struct sk_buff *skb)
{
	struct net_bridge_port *p = rcu_dereference(skb->dev->br_port);

	if (p)
		br_fdb_update(p->br, p, eth_hdr(skb)->h_source);
	return 0;	 /* process further */
}

/* Does address match the link local multicast address.
 * 01:80:c2:00:00:0X
 */
static inline int is_link_local(const unsigned char *dest)
{
	__be16 *a = (__be16 *)dest;
	static const __be16 *b = (const __be16 *)br_group_address;
	static const __be16 m = cpu_to_be16(0xfff0);

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | ((a[2] ^ b[2]) & m)) == 0;
}

/*
 * Called via br_handle_frame_hook.
 * Return NULL if skb is handled
 * note: already called with rcu_read_lock (preempt_disabled)
 */
struct sk_buff *br_handle_frame(struct net_bridge_port *p, struct sk_buff *skb)
{
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	int (*rhook)(struct sk_buff *skb);

	if (!is_valid_ether_addr(eth_hdr(skb)->h_source))
		goto drop;

	skb = skb_share_check(skb, GFP_ATOMIC);
	if (!skb)
		return NULL;

#ifdef CONFIG_ATHRS_HW_NAT
        skb->ath_hw_nat_fw_flags = 1;
#endif

	if (unlikely(is_link_local(dest))) {
		/* Pause frames shouldn't be passed up by driver anyway */
		if (skb->protocol == htons(ETH_P_PAUSE))
			goto drop;

		/* If STP is turned off, then forward */
		if (p->br->stp_enabled == BR_NO_STP && dest[5] == 0)
			goto forward;

		if (NF_HOOK(PF_BRIDGE, NF_BR_LOCAL_IN, skb, skb->dev,
			    NULL, br_handle_local_finish))
			return NULL;	/* frame consumed by filter */
		else
			return skb;	/* continue processing */
	}

forward:
	switch (p->state) {
	case BR_STATE_FORWARDING:
		rhook = rcu_dereference(br_should_route_hook);
		if (rhook != NULL) {
			if (rhook(skb))
				return skb;
			dest = eth_hdr(skb)->h_dest;
		}
		/* fall through */
	case BR_STATE_LEARNING:
		if (!compare_ether_addr(p->br->dev->dev_addr, dest))
			skb->pkt_type = PACKET_HOST;

		NF_HOOK(PF_BRIDGE, NF_BR_PRE_ROUTING, skb, skb->dev, NULL,
			br_handle_frame_finish);
		break;
	default:
drop:
		kfree_skb(skb);
	}
	return NULL;
}
