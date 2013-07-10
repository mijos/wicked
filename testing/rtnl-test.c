#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <signal.h>
#include <netinet/in.h>
#include <netlink/netlink.h>

#include <wicked/logging.h>
#include <wicked/socket.h>
#include <wicked/netinfo.h>
#include <wicked/wireless.h>
#include <wicked/ipv6.h>

#include "netinfo_priv.h"


/*
 * ====================================================================
 */
static int		hup_sig;
static int		term_sig;
static void		catch_hup_signal(int);
static void		catch_term_signal(int);
static const char *	program_name;

static void		rtnl_test_interface_event(ni_netdev_t *, ni_event_t);
static void		rtnl_test_interface_addr_event(ni_netdev_t *, ni_event_t,
							const ni_address_t *);
static void		rtnl_test_interface_prefix_event(ni_netdev_t *, ni_event_t,
							const ni_ipv6_ra_pinfo_t *);
static void		rtnl_test_interface_ndopt_event(ni_netdev_t *, ni_event_t);

int main(int argc, char **argv)
{
	signal(SIGHUP,  catch_hup_signal);
	signal(SIGINT,  catch_term_signal);
	signal(SIGTERM, catch_term_signal);

	program_name = ni_basename(argv[0]);

	ni_enable_debug("all");
	ni_log_level_set("debug3");

	if (ni_init(program_name) < 0)
		return 1;

	ni_srandom();
	ni_wireless_set_scanning(FALSE);

	if( ni_global_state_handle(1) == NULL)
		ni_fatal("cannot refresh global state!");

	ni_server_listen_interface_events(rtnl_test_interface_event);
	ni_server_enable_interface_addr_events(rtnl_test_interface_addr_event);
	ni_server_enable_interface_prefix_events(rtnl_test_interface_prefix_event);
	ni_server_enable_interface_nduseropt_events(rtnl_test_interface_ndopt_event);

	while (!term_sig) {
		long timeout;

		if (hup_sig) {
			hup_sig = 0;
			ni_trace("=== HUP ===================================");
			__ni_system_refresh_interfaces(ni_global_state_handle(0));
		}

		timeout = ni_timer_next_timeout();
		if (ni_socket_wait(timeout) < 0)
			ni_fatal("ni_socket_wait failed");
	}

	ni_trace("caught signal %u, exiting", term_sig);

	ni_server_deactivate_interface_events();
	ni_socket_deactivate_all();

	ni_trace("bye!");

	return 0;
}

static void
catch_hup_signal(int signr)
{
	hup_sig = signr;
	signal(SIGHUP,  catch_hup_signal);
}

static void
catch_term_signal(int sig)
{
	term_sig = sig;
	signal(sig,  catch_term_signal);
}

/*
 * ====================================================================
 */
static void
rtnl_test_interface_event(ni_netdev_t *dev, ni_event_t event)
{
	ni_netconfig_t *nc = ni_global_state_handle(0);
	ni_address_t *ap;

	ni_trace("%s[%u]: received interface event: %s",
		dev->name, dev->link.ifindex, ni_event_type_to_name(event));

	switch (event) {
		case NI_EVENT_NETWORK_UP:
			__ni_system_refresh_interface(nc, dev);
			for (ap = dev->addrs; ap ; ap = ap->next) {
				ni_trace("%s[%u]: found address %s: flags%s%s [%02x]",
					dev->name, dev->link.ifindex,
					ni_sockaddr_print(&ap->local_addr),
					(ap->flags & IFA_F_TENTATIVE)?   " tentative" : "",
					(ap->flags & IFA_F_DADFAILED)?   " dadfailed" : "",
					(unsigned)ap->flags);
			}
		break;

		default:
		break;
	}
}

static void
rtnl_test_interface_addr_event(ni_netdev_t *dev, ni_event_t ev, const ni_address_t *ap)
{
	ni_trace("%s[%u]: received interface address event: %s %s",
		dev->name, dev->link.ifindex, ni_event_type_to_name(ev),
		ni_sockaddr_print(&ap->local_addr));
}

static void
rtnl_test_interface_prefix_event(ni_netdev_t *dev, ni_event_t ev,const ni_ipv6_ra_pinfo_t *pi)
{
	ni_trace("%s[%u]: received interface prefix info event: %s",
		dev->name, dev->link.ifindex, ni_event_type_to_name(ev));

	ni_trace("%s[%u]: RA<%s>, Prefix %s/%u <%s,%s> lifetime[%u,%u]",
		dev->name, dev->link.ifindex,
		(dev->ipv6 && dev->ipv6->radv.managed_addr ? "managed-address" :
		 (dev->ipv6 && dev->ipv6->radv.other_config ? "other-config" :
		  "unmanaged")),
		ni_sockaddr_print(&pi->prefix), pi->length,
		(pi->on_link ? "onlink" : "not-onlink"),
		(pi->autoconf ? "autoconf" : "no-autoconf"),
		pi->lifetime.preferred_lft, pi->lifetime.valid_lft);
}

static void
rtnl_test_interface_ndopt_event(ni_netdev_t *dev, ni_event_t ev)
{
	unsigned int i;

	ni_trace("%s[%u]: received interface nd user opt event: %s",
		dev->name, dev->link.ifindex, ni_event_type_to_name(ev));

	switch (ev) {
	case NI_EVENT_RDNSS_UPDATE:
		if (!ni_netdev_get_ipv6(dev) || !dev->ipv6->radv.rdnss)
			return;

		ni_trace("%s[%u]: rdnss lifetime %u",
			dev->name, dev->link.ifindex,
			dev->ipv6->radv.rdnss->lifetime);

		for (i = 0; i < dev->ipv6->radv.rdnss->addrs.count; ++i) {
			ni_sockaddr_t *srv = &dev->ipv6->radv.rdnss->addrs.data[i];
			ni_trace("%s[%u]: rdnss[%u] server %s",
				dev->name, dev->link.ifindex,
				i, ni_sockaddr_print(srv));
		}
		break;
	default:
		break;
	}
}

