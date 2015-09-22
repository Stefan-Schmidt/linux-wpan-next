#ifndef __6LOWPAN_GHC_H
#define __6LOWPAN_GHC_H

#include <linux/skbuff.h>

/**
 * lowpan_ghc_decompression
 *
 * @skb: skb of 6LoWPAN header to read ghc and replace header.
 * @needed:
 */
int lowpan_ghc_decompression(struct sk_buff *skb, size_t needed, struct ipv6hdr *hdr);

#endif /* __6LOWPAN_GHC_H */
