This guide will help you build a NAPT66 router quickly if you have complete knowledge on IPv4 NAT and IPv6.

# 1. First, we have to modify the Linux kernel codes. #

Since kernel team don't think NAPT66 is a good idea, they have made some limitations on IPv6. That is, an IPv6 device couldn't be a router and a host meanwhile.

The simplest way to cancel this limitation is deleting two lines in 'net/ipv6/ip6\_output.c' and rebuilding the kernel.

_if (net->ipv6.devconf\_all->forwarding == 0)_

_goto error;_



# 2. Build the NAPT66 module and install it. #

Download the codes from SourceForge.net or our site napt66.buptcs.cn.

_# make_

_# insmod napt66.ko wan\_if=eth0_

The 'eth0' is an interface connecting the external IPv6 network (e.g. provided by an ISP). Next, set an IPv6 address 'fc00:0101:0101::1' (just like '192.168.1.1' in IPv4) on 'eth1', the interface connecting the internal IPv6 network.

The device can do NAPT66 now but we have to manually edit the IPv6 connection on each host. Actually Radvd and Dnsmasq can help us do that.


# 3. Install Radvd. #

IPv6 has involved two schemes to automatically configure hosts: the stateful one DHCPv6, and the stateless one Router Advertisement. Since the stateless one is more compatible, we had better turn to Radvd.

To cooperate the modification of Linux kernel, we have to modify the codes of Radvd too. Delete four lines in 'radvd-1.6/radvd.c'.

_int_

_check\_ip6\_forwarding(void)_

_{_

_......_

_//    if (value != 1) {_

_//        flog(LOG\_DEBUG, "IPv6 forwarding setting is: %u, should be 1", value);_

_//        return(-1);_

_//    }_

_......_

_}_

Then build, install and configure Radvd. You can take this as a reference.

_#/etc/radvd.conf_

_interface eth1_

_{_

_AdvSendAdvert on;_

_MinRtrAdvInterval 5;_

_MaxRtrAdvInterval 10;_

_#以下两个参数就是以前提到的M和O标记，作用是通知内网计算机网络的地址配置方式_

_AdvManagedFlag off;_

_AdvOtherConfigFlag off;_

_AdvDefaultPreference high;_

_#前缀信息_

_prefix fc00:0101:0101::/64_

_{_

_AdvOnLink on;_

_AdvAutonomous on;_

_AdvRouterAddr on;_

_};_

_#DNS信息_

_RDNSS fc00:0101:0101::1_

_{_

_AdvRDNSSPreference 15;_

_AdvRDNSSOpen on;_

_};_

_};_


# 4. Install Dnsmasq. #
Dnsmasq is a DNS forwarder for NAT firewalls. You have to Install Dnsmasq since we have define DNS server as 'fc00:0101:0101::1' in 'radvd.conf'. The newest version (maybe dndnsmasq-2.56test18 and later) supports IPv6 well. Build and install Dnsmasq now.

In addition, an advanced usage of Dnsmasq can help us redefine the records of domains.

For example, Dnsmasq will return a specific AAAA record but forward the A record query to the external DNS server due to the configuration below.

_#/etc/dnsmasq.conf_

_address=/www.youtube.com/2404:6800:8005::65_

_server=/www.youtube.com/#_




If your NAPT6 router still doesn't work, contact me please.
If you have no time to configure NAPT66, you can use a firmware provided by us. The firmware is NAPT66-ready for Broadcom BCM63xx device (e.g. Shanghai Bell RG100A).


Weilin Xu <mzweilin at gmail.com>

Beijing Univ. of Posts & Telecom.

Feb 23, 2011
