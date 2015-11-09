/*
 *	Forwarding decision
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
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/netfilter_bridge.h>
#include "br_private.h"
#include "br_igmp.h"

#ifdef CONFIG_DNI_MCAST_TO_UNICAST
#include <linux/ip.h>
static inline struct __mac_cache *
find_mac_cache(unsigned long sip, unsigned long gip)
{
      int i;
      for (i = 0; i < MCAST_ENTRY_SIZE; i++)
      {
              if (mac_cache[i].valid)
              {
                      if (mac_cache[i].sip == sip)
                              return &mac_cache[i];
              }
      }
      return NULL;
}

static inline struct __mgroup_mbr_list *
mbr_find(struct __mgroup_list *mg, unsigned long sip)
{
      struct __mgroup_mbr_list *ptr = mg->member;

      while (ptr)
      {
              if (ptr->sip == sip)
                      break;
              ptr = ptr->next;
      }

      return ptr;
}

static inline struct __mgroup_mbr_list *
mbr_add(struct __mac_cache *cache)
{
      struct __mgroup_mbr_list *ptr = NULL;

      ptr = kmalloc( sizeof(struct __mgroup_mbr_list), GFP_ATOMIC);
      if (likely(ptr))
      {
              memcpy(ptr->mac, cache->mac, 6);
              ptr->sip = cache->sip;
              ptr->dev = cache->dev;
              ptr->next = NULL;
      }
      return ptr;
}

static inline void
mbr_del(struct __mgroup_list *mg, unsigned long sip)
{
      struct __mgroup_mbr_list *ptr = mg->member, *preptr = NULL;

      while (ptr)
      {
              if (ptr->sip == sip)
                      break;
              preptr = ptr;
              ptr = ptr->next;
      }
      if (preptr)
              preptr->next = ptr->next;
      else if (ptr->next)
              mg->member = ptr->next;
      else
              mg->member = NULL;

      kfree(ptr);
      return;
}

static inline struct __mgroup_list *
mgroup_find(unsigned long gip)
{
      struct __mgroup_list *ptr = mhead;
      while (ptr)
      {
              if (ptr->gip == gip)
                      break;
              ptr = ptr->next;
      }

      return ptr;
}

static inline struct __mgroup_list *
mgroup_add(unsigned long sip, unsigned long gip)
{
      struct __mgroup_list *ptr = mhead, *tmp;
      struct __mac_cache *cache = NULL;
      struct __mgroup_mbr_list *msource = NULL;

      while (ptr)
              ptr = ptr->next;
      cache = find_mac_cache(sip, gip);
      if (cache)
              ptr = kmalloc( sizeof(struct __mgroup_list), GFP_ATOMIC);

      if (likely(ptr))
      {
              ptr->gip = gip;
              msource = mbr_add(cache);
              if (unlikely(!msource))
              {
                      kfree(ptr);
                      return NULL;
              }
              ptr->member = msource;
              ptr->next = NULL;

              if (mhead)
                      INSERT_TO_TAIL(mhead, ptr, tmp);
              else
                      mhead = ptr;
              cache->valid = 0;
      }
      else
              ptr = NULL;
      return ptr;
}

static inline void
mgroup_del(unsigned long gip)
{
      struct __mgroup_list *ptr = mhead, *preptr = NULL;

      while (ptr)
      {
              if (ptr->gip == gip)
                      break;
              preptr = ptr;
              ptr = ptr->next;
      }
      if (preptr)
              preptr->next = ptr->next;
      else if (ptr->next)
              mhead = ptr->next;
      else
              mhead = NULL;

      kfree(ptr);
      return ;
}

void
proc_mcast_entry(char cmd, unsigned long sip , unsigned long gip)
{
      struct __mgroup_list *ptr = NULL;
      struct __mgroup_mbr_list *mptr = NULL, *tmp;
      struct __mac_cache * cache = NULL;
      if (cmd == 'a')
      {
              ptr = mgroup_find(gip);
              if (ptr)
              {
                      mptr = mbr_find(ptr, sip);
                      if (!mptr)
                      {
                              cache = find_mac_cache(sip, gip);
                              if (cache)
                              {
                                      mptr = mbr_add(cache);
                                      if (mptr)
                                              INSERT_TO_TAIL(ptr->member, mptr, tmp);
                              }
                      }
              }
              else
                      ptr = mgroup_add(sip, gip);
      }
      else
      {
              ptr = mgroup_find(gip);
              if (ptr)
              {
                      mptr = mbr_find(ptr, sip);
                      if (mptr)
                      {
                              mbr_del(ptr, sip);
                              if (!ptr->member)
                                      mgroup_del(gip);
                      }
              }
      }
      return;
}

#define LOCAL_CONTROL_START 0xE0000000
#define LOCAL_CONTROL_END   0XE00000FF
#define SSDP    0XEFFFFFFA

static inline int 
not_ctrl_group(unsigned long ip)
{
      if ( (ip >= LOCAL_CONTROL_START) &&( ip <= LOCAL_CONTROL_END))
              return 0;
      else if (ip == SSDP)
              return 0;
      return 1;
}

static inline struct sk_buff *
modifyMcast2Ucast(struct sk_buff *skb,unsigned char *mac)
{
      struct sk_buff *mSkb;

      mSkb=skb_copy(skb,GFP_ATOMIC);
      struct ethhdr *ethernet = (struct ethhdr *)mSkb->mac_header;
      memcpy(ethernet->h_dest, mac, 6);

      return mSkb;
}

int mcast_set_read( char *page, char **start, off_t off,
                                int count, int *eof, void *data )
{
      struct __mgroup_list *ptr = mhead;
      struct __mgroup_mbr_list *mptr;
      int i;
      while (ptr)
      {
              mptr = ptr->member;
              while (mptr)
              {
                      printk("client : [%8x] join group [%8x]\n", mptr->sip, ptr->gip);
                      printk("mode : %s\n", (mptr->set.mode)?"Include":"Exclude");
                      for (i = 0; i < mptr->set.num; i++)
                              printk("source%d : %8x\n", i, mptr->set.sip[i]);
                      mptr = mptr->next;
              }
              ptr = ptr->next;
      }

      return 0;
}

ssize_t mcast_set_write( struct file *filp, const char __user *buff,
                                         unsigned long len, void *data )
{
      char line[256], *p, *e, i;
      struct __mgroup_list *ptr = NULL;
      struct __mgroup_mbr_list *mptr = NULL;
      unsigned long sip, gip;
      struct source_set set;
      if (copy_from_user( line, buff, len ))
              return -EFAULT;
      line[len] = 0;
      // ip gip mode num source1 source2
      p = line;
      e = strsep(&p, " ");
      if (!e)
              return len;
      sip = a2n(e);

      e = strsep(&p, " ");
      if (!e)
              return len;
      gip = a2n(e);

      memset(&set, 0, sizeof(set));
      e = strsep(&p, " ");
      if (!e)
              return len;
      set.mode = *e - '0';

      e = strsep(&p, " ");
      if (!e)
              return len;
      set.num = *e - '0';

      for (i = 0; i < set.num; i++)
      {
              e = strsep(&p, " ");
              if (!e)
                      return len;
              set.sip[i] = a2n(e);
      }

      ptr = mgroup_find(gip);
      if (ptr)
      {
              mptr = mbr_find(ptr, sip);
              if (mptr)
                      memcpy(&mptr->set, &set, sizeof(set));

      }
      return len;
}

static inline int
include_check(unsigned long sip, struct source_set *set)
{
      int i;
      for (i = 0; i < set->num; i++)
              if (sip == set->sip[i])
                      return 1;
      return 0;
}

static inline int
exclude_check(unsigned long sip, struct source_set *set)
{
      int i;
      for (i = 0; i < set->num; i++)
              if (sip == set->sip[i])
                      return 0;
      return 1;
}

static inline int
pass_check(unsigned long sip, struct __mgroup_mbr_list *mbr)
{
      if (mbr->set.mode)
              return include_check(sip, &mbr->set);
      else
              return exclude_check(sip, &mbr->set);
}
#endif

#ifdef CONFIG_DNI_IPTV_PORT_SUPPORT
void iptv_port_print_mgroup(void)
{
	int i;
	uint32_t ip;
	printk("------ IPTV Port Multicast Groups ------\n");
	for(i=0; i<IPTV_PORT_GROUP_MAX; i++)
	{
		ip = iptv_port_mgroup[i].group_ip;
		if(ip != 0)
		{
			printk("%5s - %d.%d.%d.%d - %3d(s)\n", iptv_port_mgroup[i].port_name, (ip>>24)&0xff, (ip>>16)&0xff, 
									(ip>>8)&0xff, (ip)&0xff, iptv_port_mgroup[i].group_time);
		}
	}
}

static int iptv_port_find_mgroup(char *pname, uint32_t ip, int update_time)
{
	int i;
	for(i=0; i<IPTV_PORT_GROUP_MAX; i++)
	{
		if(strcmp(iptv_port_mgroup[i].port_name, pname) == 0 && iptv_port_mgroup[i].group_ip == ip)
		{
			if(update_time == 1)
				iptv_port_mgroup[i].group_time = PORT_GMI;
			return 1;
		}
	}
	return 0;
}

static void iptv_port_add_mgroup(char *pname, uint32_t ip)
{
	int i;
	for(i=0; i<IPTV_PORT_GROUP_MAX; i++)
	{
		if(iptv_port_mgroup[i].group_ip == 0)
		{
			iptv_port_mgroup[i].group_ip = ip;
			strncpy(iptv_port_mgroup[i].port_name, pname, 15);
			iptv_port_mgroup[i].group_time = PORT_GMI;
			break;
		}
	}
}

static void iptv_port_del_mgroup(char *pname, uint32_t ip)
{
	int i;
	for(i=0; i<IPTV_PORT_GROUP_MAX; i++)
	{
		if(strcmp(iptv_port_mgroup[i].port_name, pname) == 0 && iptv_port_mgroup[i].group_ip == ip)
		{
			iptv_port_mgroup[i].group_ip = 0;
			*(iptv_port_mgroup[i].port_name) = '\0';
			iptv_port_mgroup[i].group_time = 0;
			break;
		}
	}
}

static void iptv_port_detect_mgroup(struct net_bridge *br, const struct net_bridge_port *p, const struct sk_buff *skb)
{
	struct iphdr *igmp_iph;
	unsigned char *igmp_type;
	unsigned short igmpv3_numsrc;
        unsigned short igmpv3_numgrps;
	unsigned char igmpv3_type;
	int i;
	uint32_t gip;
	igmpr_t *igmpv12_report;
	igmp_report_t *igmpv3_report;
	igmp_iph = (struct iphdr *)skb->network_header;
	if(igmp_iph->protocol == IPPROTO_IGMP && igmp_iph->daddr != SSDP)
	{
		igmp_type = (unsigned char *)igmp_iph + (igmp_iph->ihl << 2);
		switch(*igmp_type)
		{
			case IGMP_V1_MEMBERSHIP_REPORT:
			case IGMP_V2_MEMBERSHIP_REPORT:
				igmpv12_report = (igmpr_t *)igmp_type;
				gip = *((uint32_t *)&igmpv12_report->igmpr_group);
				if(iptv_port_enable > 1)
					printk("%s+%d: --- IGMP_V2: %s Join %d.%d.%d.%d ---\n",__FILE__,__LINE__,skb->dev->name,(gip>>24)&0xff,
								(gip>>16)&0xff,(gip>>8)&0xff,(gip)&0xff);
				if(!iptv_port_find_mgroup(skb->dev->name, gip, 1))
				{
					iptv_port_add_mgroup(skb->dev->name, gip);
				}
				break;
			case IGMP_V2_MEMBERSHIP_LEAVE:
				igmpv12_report = (igmpr_t *)igmp_type;
				gip = *((uint32_t *)&igmpv12_report->igmpr_group);
				if(iptv_port_enable > 1)
					printk("%s+%d: --- IGMP_V2: %s Leave %d.%d.%d.%d ---\n",__FILE__,__LINE__,skb->dev->name,
								(gip>>24)&0xff,(gip>>16)&0xff,(gip>>8)&0xff,(gip)&0xff);
				if(iptv_port_find_mgroup(skb->dev->name, gip, 0))
				{
					iptv_port_del_mgroup(skb->dev->name, gip);
				}
				break;
			case IGMP_V3_MEMBERSHIP_REPORT:
				igmpv3_report = (igmp_report_t *)igmp_type;
				igmpv3_numgrps = ntohs(igmpv3_report->igmpr_numgrps);
				for(i=0; i<igmpv3_numgrps; i++)
				{
					igmpv3_type   = (unsigned char)igmpv3_report->igmpr_group[i].igmpg_type;
					igmpv3_numsrc = ntohs(igmpv3_report->igmpr_group[i].igmpg_numsrc);
					switch(igmpv3_type)
					{
						case 1:
						case 2:
						case 4:
						case 5:
						case 6:
							gip = *((uint32_t *)&igmpv3_report->igmpr_group[i].igmpg_group);
							if(iptv_port_enable > 1)
								printk("%s+%d: --- IGMP_V3: %s Join %d.%d.%d.%d ---\n",__FILE__,__LINE__,
										skb->dev->name,(gip>>24)&0xff,(gip>>16)&0xff,(gip>>8)&0xff,(gip)&0xff);
							if(!iptv_port_find_mgroup(skb->dev->name, gip, 1))
							{
								iptv_port_add_mgroup(skb->dev->name, gip);
							}
							break;
						case 3:
							if(igmpv3_numsrc == 0)
							{
								gip = *((uint32_t *)&igmpv3_report->igmpr_group[i].igmpg_group);
								if(iptv_port_enable > 1)
									printk("%s+%d: --- IGMP_V3: %s Leave %d.%d.%d.%d ---\n",__FILE__,__LINE__,
										skb->dev->name,(gip>>24)&0xff,(gip>>16)&0xff,(gip>>8)&0xff,(gip)&0xff);
								if(iptv_port_find_mgroup(skb->dev->name, gip, 0))
								{
									iptv_port_del_mgroup(skb->dev->name, gip);
								}
							}else{
								gip = *((uint32_t *)&igmpv3_report->igmpr_group[i].igmpg_group);
								if(iptv_port_enable > 1)
									printk("%s+%d: --- IGMP_V3: %s Join %d.%d.%d.%d ---\n",__FILE__,__LINE__,
										skb->dev->name,(gip>>24)&0xff,(gip>>16)&0xff,(gip>>8)&0xff,(gip)&0xff);
								if(!iptv_port_find_mgroup(skb->dev->name, gip, 1))
								{
									iptv_port_add_mgroup(skb->dev->name, gip);
								}
							}
							break;
						default:
							break;
					}
				}
				break;
			default:
				break;
		}
	}

}

static void iptv_port_timer_func(void)
{
	int i;
	mod_timer(&iptv_port_timer,(jiffies+HZ));
	for(i=0; i<IPTV_PORT_GROUP_MAX; i++)
	{
		--iptv_port_mgroup[i].group_time;
		if(iptv_port_mgroup[i].group_time == 0)
		{
			iptv_port_mgroup[i].group_ip = 0;
			*(iptv_port_mgroup[i].port_name) = '\0';
		}
	}
}

void iptv_port_timer_thread()
{
	init_timer(&iptv_port_timer);
	iptv_port_timer.function=iptv_port_timer_func;
	iptv_port_timer.expires=jiffies+HZ;
	iptv_port_timer.data=NULL;
	add_timer(&iptv_port_timer);
}
#endif

/* Don't forward packets to originating port or forwarding diasabled */
static inline int should_deliver(const struct net_bridge_port *p,
				 const struct sk_buff *skb)
{
#ifdef CONFIG_DNI_MULTI_LAN_SUPPORT
      // if skb is from/to eth1/ath1/ath0, to deliver skb, do nothing for not going into else if.
      if(skb->dev->name[3] == '1' || p->dev->name[3] == '1') {
      }
      // if skb is from eth1/eth2/eth3/eth4 and destination is eth1/eth2/eth3/eth4, to drop skb
      else if (skb->dev->name[0] == 'e' &&
                  (skb->dev->name[0] == p->dev->name[0]))
              return 0;
#endif
	return (skb->dev != p->dev && p->state == BR_STATE_FORWARDING);
}

static inline unsigned packet_length(const struct sk_buff *skb)
{
	return skb->len - (skb->protocol == htons(ETH_P_8021Q) ? VLAN_HLEN : 0);
}

int br_dev_queue_push_xmit(struct sk_buff *skb)
{
	/* drop mtu oversized packets except gso */
	if (packet_length(skb) > skb->dev->mtu && !skb_is_gso(skb))
		kfree_skb(skb);
	else {
		/* ip_refrag calls ip_fragment, doesn't copy the MAC header. */
		if (nf_bridge_maybe_copy_header(skb))
			kfree_skb(skb);
		else {
			skb_push(skb, ETH_HLEN);

			dev_queue_xmit(skb);
		}
	}

	return 0;
}

int br_forward_finish(struct sk_buff *skb)
{
	return NF_HOOK(PF_BRIDGE, NF_BR_POST_ROUTING, skb, NULL, skb->dev,
		       br_dev_queue_push_xmit);

}

static void __br_deliver(const struct net_bridge_port *to, struct sk_buff *skb)
{
	skb->dev = to->dev;
#ifdef CONFIG_BRIDGE_NETGEAR_ACL
	struct net_bridge *br = to->br;
	if (!br_acl_should_pass(br, skb, ACL_CHECK_DST)) {
		br->dev->stats.tx_dropped++;
		kfree_skb(skb);
		return;
	}
#endif
	NF_HOOK(PF_BRIDGE, NF_BR_LOCAL_OUT, skb, NULL, skb->dev,
			br_forward_finish);
}

static void __br_forward(const struct net_bridge_port *to, struct sk_buff *skb)
{
	struct net_device *indev;

	if (skb_warn_if_lro(skb)) {
		kfree_skb(skb);
		return;
	}

	indev = skb->dev;
	skb->dev = to->dev;
	skb_forward_csum(skb);

#ifdef CONFIG_BRIDGE_NETGEAR_ACL
	struct net_bridge *br = to->br;
	if (!br_acl_should_pass(br, skb, (ACL_CHECK_SRC | ACL_CHECK_DST))) {
		br->dev->stats.tx_dropped++;
		kfree_skb(skb);
		return;
	}
#endif
	NF_HOOK(PF_BRIDGE, NF_BR_FORWARD, skb, indev, skb->dev,
			br_forward_finish);
}

/* called with rcu_read_lock */
void br_deliver(const struct net_bridge_port *to, struct sk_buff *skb)
{
	if (should_deliver(to, skb)) {
		__br_deliver(to, skb);
		return;
	}

	kfree_skb(skb);
}

/* called with rcu_read_lock */
void br_forward(const struct net_bridge_port *to, struct sk_buff *skb)
{
	if (should_deliver(to, skb)) {
		__br_forward(to, skb);
		return;
	}

	kfree_skb(skb);
}

/* called under bridge lock */
static void br_flood(struct net_bridge *br, struct sk_buff *skb,
	void (*__packet_hook)(const struct net_bridge_port *p,
			      struct sk_buff *skb))
{
	struct net_bridge_port *p;
	struct net_bridge_port *prev;
	struct ethhdr *ethernet = (struct ethhdr *)skb->mac_header;
#ifdef CONFIG_DNI_IPTV_PORT_SUPPORT
        unsigned char *iptv_dest_mac;
        struct iphdr *iptv_iph;
        iptv_dest_mac = ethernet->h_dest;
        iptv_iph = (struct iphdr *)skb->network_header;
#endif
	prev = NULL;


	list_for_each_entry_rcu(p, &br->port_list, list) {
		if (should_deliver(p, skb)) {
#ifdef CONFIG_DNI_IPTV_PORT_SUPPORT
                        if(iptv_port_enable && !strncmp(br->dev->name, "br1", 3) && MULTICAST_MAC(iptv_dest_mac))
                        {
                                if(!strncmp(p->dev->name, "eth0", 4))//to WAN port
                                {
                                        if(strncmp(skb->dev->name, "br1", 3) != 0)//from IPTV port
                                                iptv_port_detect_mgroup(br, p, skb);
                               }
                                else if(!strncmp(skb->dev->name, "eth0", 4) && not_ctrl_group(iptv_iph->daddr))//from WAN port
                                {
                                        if(strncmp(p->dev->name, "br1", 3) != 0)//to IPTV port
                                                if(!iptv_port_find_mgroup(p->dev->name, iptv_iph->daddr, 0))
                                                        continue;
                                }
                        }
#endif
#ifdef CONFIG_DNI_MCAST_TO_UNICAST
                      if (prev)
                      {
                              struct sk_buff *skb2;

                              if (igmp_snoop_enable && prev->dev->name[0] == 'a' && !strncmp(br->dev->name, "br0", 3))
                              {
				      unsigned char *dest = ethernet->h_dest;
                                      struct iphdr *iph = (struct iphdr *)skb->network_header;
                                      if ( MULTICAST_MAC(dest) && not_ctrl_group(iph->daddr))
                                      {
                                              struct __mgroup_list *mg = NULL;
                                              struct __mgroup_mbr_list *mbr;
                                              mg = mgroup_find(iph->daddr);
                                              if (mg)
                                              {
                                                      mbr = mg->member;
                                                      while (mbr)
                                                      {
                                                              if ((mbr->dev == prev->dev) &&
                                                                      pass_check(iph->saddr, mbr))
                                                              {
                                                                      if((skb2=modifyMcast2Ucast(skb, mbr->mac)) == NULL)
                                                                      {
                                                                              br->dev->stats.tx_dropped++;
                                                                              kfree_skb(skb);
                                                                              return;
                                                                      }
                                                                      __packet_hook(prev, skb2);
                                                              }
                                                              mbr = mbr->next;
                                                      }
                                              }

                                              prev = p;
                                              continue;
                                      }
                              }

                              if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
                                      br->dev->stats.tx_dropped++;
                                      kfree_skb(skb);
                                      return;
                              }

                              __packet_hook(prev, skb2);

                      }
                      prev = p;
#else
			if (prev != NULL) {
				struct sk_buff *skb2;

				if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
					br->dev->stats.tx_dropped++;
					kfree_skb(skb);
					return;
				}

				__packet_hook(prev, skb2);
			}

			prev = p;
#endif
		}
	}
	if (prev != NULL) {
#ifdef CONFIG_DNI_MCAST_TO_UNICAST
              struct sk_buff *skb2;

              if (prev->dev->name[0] == 'a' && !strncmp(br->dev->name, "br0", 3))
              {
                      unsigned char *dest = ethernet->h_dest;
                      struct iphdr *iph = (struct iphdr *)skb->network_header;
                      if ( MULTICAST_MAC(dest) && not_ctrl_group(iph->daddr))
                      {
                              struct __mgroup_list *mg = NULL;
                              struct __mgroup_mbr_list *mbr;
                              mg = mgroup_find(iph->daddr);
                              if (mg)
                              {
                                      mbr = mg->member;
                                      while (mbr)
                                      {
                                              if ((mbr->dev == prev->dev)&&
                                                      pass_check(iph->saddr, mbr))
                                              {
                                                      if((skb2=modifyMcast2Ucast(skb, mbr->mac)) == NULL)
                                                      {
                                                              br->dev->stats.tx_dropped++;
                                                              kfree_skb(skb);
                                                              return;
                                                      }
                                                      __packet_hook(prev, skb2);
                                              }
                                              mbr = mbr->next;
                                      }
                                      kfree_skb(skb);
                                      return;
                              }
                      }
              }
#endif
		__packet_hook(prev, skb);
		return;
	}

	kfree_skb(skb);
}


/* called with rcu_read_lock */
void br_flood_deliver(struct net_bridge *br, struct sk_buff *skb)
{
	br_flood(br, skb, __br_deliver);
}

/* called under bridge lock */
void br_flood_forward(struct net_bridge *br, struct sk_buff *skb)
{
	br_flood(br, skb, __br_forward);
}
