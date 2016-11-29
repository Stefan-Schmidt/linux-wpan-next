Introduction
============
The IEEE 802.15.4 working group focuses on standardization of the bottom
two layers: Medium Access Control (MAC) and Physical access (PHY). And there
are mainly two options available for upper layers:

- ZigBee - proprietary protocol from the ZigBee Alliance
- 6LoWPAN - IPv6 networking over low rate personal area networks

The goal of the Linux-wpan is to provide a complete implementation
of the IEEE 802.15.4 and 6LoWPAN protocols. IEEE 802.15.4 is a stack
of protocols for organizing Low-Rate Wireless Personal Area Networks.

The stack is composed of three main parts:

- IEEE 802.15.4 layer;  We have chosen to use plain Berkeley socket API,
  the generic Linux networking stack to transfer IEEE 802.15.4 data
  messages and a special protocol over netlink for configuration/management
- MAC - provides access to shared channel and reliable data delivery
- PHY - represents device drivers


Socket API
----------

c:function:: int sd = socket(PF_IEEE802154, SOCK_DGRAM, 0);

The address family, socket addresses etc. are defined in the
include/net/af_ieee802154.h header or in the special header
in the userspace package (see either http://wpan.cakelab.org/ or the
git tree at https://github.com/linux-wpan/wpan-tools).


6LoWPAN Linux implementation
----------------------------

The IEEE 802.15.4 standard specifies an MTU of 127 bytes, yielding about 80
octets of actual MAC payload once security is turned on, on a wireless link
with a link throughput of 250 kbps or less.  The 6LoWPAN adaptation format
[RFC4944] was specified to carry IPv6 datagrams over such constrained links,
taking into account limited bandwidth, memory, or energy resources that are
expected in applications such as wireless Sensor Networks.  [RFC4944] defines
a Mesh Addressing header to support sub-IP forwarding, a Fragmentation header
to support the IPv6 minimum MTU requirement [RFC2460], and stateless header
compression for IPv6 datagrams (LOWPAN_HC1 and LOWPAN_HC2) to reduce the
relatively large IPv6 and UDP headers down to (in the best case) several bytes.

In September 2011 the standard update was published - [RFC6282].
It deprecates HC1 and HC2 compression and defines IPHC encoding format which is
used in this Linux implementation.

All the code related to 6lowpan you may find in files: net/6lowpan/*
and net/ieee802154/6lowpan/*

To setup a 6LoWPAN interface you need:
1. Add IEEE802.15.4 interface and set channel and PAN ID;
2. Add 6lowpan interface by command like:
# ip link add link wpan0 name lowpan0 type lowpan
3. Bring up 'lowpan0' interface
