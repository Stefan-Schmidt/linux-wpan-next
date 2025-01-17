/*
 * Handling of a master device, switching frames via its switch fabric CPU port
 *
 * Copyright (c) 2017 Savoir-faire Linux Inc.
 *	Vivien Didelot <vivien.didelot@savoirfairelinux.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "dsa_priv.h"

static void dsa_master_get_ethtool_stats(struct net_device *dev,
					 struct ethtool_stats *stats,
					 uint64_t *data)
{
	struct dsa_switch_tree *dst = dev->dsa_ptr;
	struct dsa_port *port = dst->cpu_dp;
	struct dsa_switch *ds = port->ds;
	const struct ethtool_ops *ops = port->orig_ethtool_ops;
	int count = 0;

	if (ops && ops->get_sset_count && ops->get_ethtool_stats) {
		count = ops->get_sset_count(dev, ETH_SS_STATS);
		ops->get_ethtool_stats(dev, stats, data);
	}

	if (ds->ops->get_ethtool_stats)
		ds->ops->get_ethtool_stats(ds, port->index, data + count);
}

static int dsa_master_get_sset_count(struct net_device *dev, int sset)
{
	struct dsa_switch_tree *dst = dev->dsa_ptr;
	struct dsa_port *port = dst->cpu_dp;
	struct dsa_switch *ds = port->ds;
	const struct ethtool_ops *ops = port->orig_ethtool_ops;
	int count = 0;

	if (ops && ops->get_sset_count)
		count += ops->get_sset_count(dev, sset);

	if (sset == ETH_SS_STATS && ds->ops->get_sset_count)
		count += ds->ops->get_sset_count(ds);

	return count;
}

static void dsa_master_get_strings(struct net_device *dev, uint32_t stringset,
				   uint8_t *data)
{
	struct dsa_switch_tree *dst = dev->dsa_ptr;
	struct dsa_port *port = dst->cpu_dp;
	struct dsa_switch *ds = port->ds;
	const struct ethtool_ops *ops = port->orig_ethtool_ops;
	int len = ETH_GSTRING_LEN;
	int mcount = 0, count;
	unsigned int i;
	uint8_t pfx[4];
	uint8_t *ndata;

	snprintf(pfx, sizeof(pfx), "p%.2d", port->index);
	/* We do not want to be NULL-terminated, since this is a prefix */
	pfx[sizeof(pfx) - 1] = '_';

	if (ops && ops->get_sset_count && ops->get_strings) {
		mcount = ops->get_sset_count(dev, ETH_SS_STATS);
		ops->get_strings(dev, stringset, data);
	}

	if (stringset == ETH_SS_STATS && ds->ops->get_strings) {
		ndata = data + mcount * len;
		/* This function copies ETH_GSTRINGS_LEN bytes, we will mangle
		 * the output after to prepend our CPU port prefix we
		 * constructed earlier
		 */
		ds->ops->get_strings(ds, port->index, ndata);
		count = ds->ops->get_sset_count(ds);
		for (i = 0; i < count; i++) {
			memmove(ndata + (i * len + sizeof(pfx)),
				ndata + i * len, len - sizeof(pfx));
			memcpy(ndata + i * len, pfx, sizeof(pfx));
		}
	}
}

int dsa_master_ethtool_setup(struct net_device *dev)
{
	struct dsa_switch_tree *dst = dev->dsa_ptr;
	struct dsa_port *port = dst->cpu_dp;
	struct dsa_switch *ds = port->ds;
	struct ethtool_ops *ops;

	ops = devm_kzalloc(ds->dev, sizeof(*ops), GFP_KERNEL);
	if (!ops)
		return -ENOMEM;

	port->orig_ethtool_ops = dev->ethtool_ops;
	if (port->orig_ethtool_ops)
		memcpy(ops, port->orig_ethtool_ops, sizeof(*ops));

	ops->get_sset_count = dsa_master_get_sset_count;
	ops->get_ethtool_stats = dsa_master_get_ethtool_stats;
	ops->get_strings = dsa_master_get_strings;

	dev->ethtool_ops = ops;

	return 0;
}

void dsa_master_ethtool_restore(struct net_device *dev)
{
	struct dsa_switch_tree *dst = dev->dsa_ptr;
	struct dsa_port *port = dst->cpu_dp;

	dev->ethtool_ops = port->orig_ethtool_ops;
	port->orig_ethtool_ops = NULL;
}
