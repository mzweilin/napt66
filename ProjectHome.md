# Readme #

视频Video：http://v.youku.com/v_show/id_XMjY4NTAwMTc2.html

文档Document：http://code.google.com/p/napt66/downloads/detail?name=2011%E5%8C%97%E9%82%AE%E5%88%9B%E6%96%B0%E5%A5%96%E5%8F%82%E8%B5%9B%E4%BD%9C%E5%93%81%20%E5%B5%8C%E5%85%A5%E5%BC%8FIPv6%20NAT%E8%B7%AF%E7%94%B1%E5%99%A8.pdf

# NAPT66 Patchset from Netfilter core team #
http://lwn.net/Articles/469653/
Patrick McHardy posted a patchset that implemented Stateful and Stateless IPv6-to-IPv6 NAT. I believe such feature will be integreted into Netiflter soon.

# Introduction #
IPv6-to-IPv6 Network Address Port Translation (NAPT66) is a stateful IPv6 NAT mechanism. Just like IPv4 NAT, NAPT66 technique makes several hosts share a public IPv6 address. As a result, NAPT66 helps to hide the private network topology and promote network security.

NAPT66 based on GNU/Linux is implemented in kernel space, which provides satisfied performance and portability. It has been ported to several open-source router firmware (e.g. OpenWrt) so that it can be run on low-end, commodity hardware (e.g. BCM63xx platform).

NAPT66 should be installed on a boundary router situated between two IPv6 networks. It performs stateful packet translation between internal IPv6 hosts and external IPv6 hosts.

NAPT66 uses Application Level Gateways (ALGs) and DNS Proxy to deal with complex applications (like active FTP and DNS). More applications can travels NAPT66 in the future.


# Tested Protocols #

·HTTP

·MMS

·FTP - active and passive

·Telnet

·ICMPv6 - echo request, echo reply，error messages(Destination Unreachable、Packet Too Big、Time Exceeded、Parameter Problem)


# Limitations #

In some certain scenarios, NAPT66 is unacceptable for these limitations.

_·Breaking the end-to-end model_

Several traditional NAT traversal techniques (e.g. ALGs, UPnP) may helps to solve the problem.

_·No fragmentation_

Avoiding fragmentation is one of the principles of IPv6. NAPT66 don’t support IPv6 fragmentation for the moment.

# Members #
Students:

许伟林(Weilin XU), 杨毅刚(Yigang YANG), 刘惠庭(Huiting LIU).
The three students come from Grade 2008, School of Computer Science, Beijing University of Posts & Telecommunications.

许广林(Guanglin XU).
We are glad to introduce the new member Xu Guanglin, a college student from Sun Yat-sen University. He has done a good job on integrating napt66 into OpenWrt project.

Mentor:

张华(Hua ZHANG).

Zhang Hua is a lecturer of Institute of Network Technology, BUPT.
# Acknowledgements #

This work is supported by the Research Innovation Fund for College Students of Beijing University of Posts and Telecommunications (Grant No. 101104537).