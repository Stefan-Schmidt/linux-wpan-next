Drivers
=======

Like with WiFi, there are several types of devices implementing IEEE 802.15.4.
1) 'HardMAC'. The MAC layer is implemented in the device itself, the device
exports a management (e.g. MLME) and data API.
2) 'SoftMAC' or just radio. These types of devices are just radio transceivers
possibly with some kinds of acceleration like automatic CRC computation and
comparation, automagic ACK handling, address matching, etc.

Those types of devices require different approach to be hooked into Linux kernel.


HardMAC
-------

See the header include/net/ieee802154_netdev.h. You have to implement Linux
net_device, with .type = ARPHRD_IEEE802154. Data is exchanged with socket family
code via plain sk_buffs. On skb reception skb->cb must contain additional
info as described in the struct ieee802154_mac_cb. During packet transmission
the skb->cb is used to provide additional data to device's header_ops->create
function. Be aware that this data can be overridden later (when socket code
submits skb to qdisc), so if you need something from that cb later, you should
store info in the skb->data on your own.

To hook the MLME interface you have to populate the ml_priv field of your
net_device with a pointer to struct ieee802154_mlme_ops instance. The fields
assoc_req, assoc_resp, disassoc_req, start_req, and scan_req are optional.
All other fields are required.


SoftMAC
-------

The MAC is the middle layer in the IEEE 802.15.4 Linux stack. This moment it
provides interface for drivers registration and management of slave interfaces.

NOTE: Currently the only monitor device type is supported - it's IEEE 802.15.4
stack interface for network sniffers (e.g. WireShark).

This layer is going to be extended soon.

See header include/net/mac802154.h and several drivers in
drivers/net/ieee802154/.


Device drivers API
------------------

The include/net/mac802154.h defines following functions:

- c:function:: struct ieee802154_dev *ieee802154_alloc_device (size_t priv_size, struct ieee802154_ops *ops)
  allocation of IEEE 802.15.4 compatible device

- c:function:: void ieee802154_free_device(struct ieee802154_dev *dev)
  freeing allocated device

- c:function:: int ieee802154_register_device(struct ieee802154_dev *dev)
  register PHY in the system

- c:function:: void ieee802154_unregister_device(struct ieee802154_dev *dev)
  freeing registered PHY

Moreover IEEE 802.15.4 device operations structure should be filled.

Fake drivers
------------

In addition there is a driver available which simulates a real device with
SoftMAC (fakelb - IEEE 802.15.4 loopback driver) interface. This option
provides a possibility to test and debug the stack without usage of real hardware.

See sources in drivers/net/ieee802154 folder for more details.
