/* vi: set sw=4 ts=4: */
#ifndef __LIBNETLINK_H__
#define __LIBNETLINK_H__ 1

#include <linux/types.h>
/* We need linux/types.h because older kernels use __u32 etc
 * in linux/[rt]netlink.h. 2.6.19 seems to be ok, though */
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_addr.h>
#include <linux/neighbour.h>

struct rtnl_handle
{
	int			fd;
	struct sockaddr_nl	local;
	struct sockaddr_nl	peer;
	uint32_t		seq;
	uint32_t		dump;
};

extern int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions);
extern void rtnl_close(struct rtnl_handle *rth);
extern int rtnl_wilddump_request(struct rtnl_handle *rth, int fam, int type);
extern int rtnl_dump_request(struct rtnl_handle *rth, int type, void *req, int len);
extern int rtnl_dump_filter(struct rtnl_handle *rth,
			int (*filter)(struct sockaddr_nl*, struct nlmsghdr *n, void*),
			void *arg1,
			int (*junk)(struct sockaddr_nl *, struct nlmsghdr *n, void *),
			void *arg2);
extern int rtnl_talk(struct rtnl_handle *rtnl, struct nlmsghdr *n, pid_t peer,
			unsigned groups, struct nlmsghdr *answer,
			int (*junk)(struct sockaddr_nl *,struct nlmsghdr *n, void *),
			void *jarg);
extern int rtnl_send(struct rtnl_handle *rth, char *buf, int);


extern int addattr32(struct nlmsghdr *n, int maxlen, int type, uint32_t data);
extern int addattr_l(struct nlmsghdr *n, int maxlen, int type, void *data, int alen);
extern int rta_addattr32(struct rtattr *rta, int maxlen, int type, uint32_t data);
extern int rta_addattr_l(struct rtattr *rta, int maxlen, int type, void *data, int alen);

extern int parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len);

extern int parse_rtattr_byindex(struct rtattr *tb[], int max, struct rtattr *rta, int len);
extern int __parse_rtattr_nested_compat(struct rtattr *tb[], int max, struct rtattr *rta, int len);

#define NLMSG_TAIL(nmsg) \
        ((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

#ifndef IFA_RTA
#define IFA_RTA(r) \
        ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))))
#endif
#ifndef IFA_PAYLOAD
#define IFA_PAYLOAD(n)  NLMSG_PAYLOAD(n,sizeof(struct ifaddrmsg))
#endif

#ifndef IFLA_RTA
#define IFLA_RTA(r) \
        ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifinfomsg))))
#endif
#ifndef IFLA_PAYLOAD
#define IFLA_PAYLOAD(n) NLMSG_PAYLOAD(n,sizeof(struct ifinfomsg))
#endif

#ifndef NDA_RTA
#define NDA_RTA(r) \
        ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif
#ifndef NDA_PAYLOAD
#define NDA_PAYLOAD(n)  NLMSG_PAYLOAD(n,sizeof(struct ndmsg))
#endif

#ifndef NDTA_RTA
#define NDTA_RTA(r) \
        ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndtmsg))))
#endif
#ifndef NDTA_PAYLOAD
#define NDTA_PAYLOAD(n) NLMSG_PAYLOAD(n,sizeof(struct ndtmsg))
#endif

#endif /* __LIBNETLINK_H__ */
