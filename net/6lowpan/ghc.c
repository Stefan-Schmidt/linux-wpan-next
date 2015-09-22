/*
 *	6LoWPAN generic header compression/decompression from RFC7400
 *
 *
 *	Authors:
 *	Stefan Schmidt <stefan@osg.samsung.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/netdevice.h>
#include <net/ipv6.h>
#include <linux/skbuff.h>

#include "nhc.h"
#include "ghc.h"

#define GHC_STOP_CODE 0x90

/* Static dictionary for decompression. RFC 7400, section 2, page 5 */
static u8 static_dict[16] = {0x16, 0xfe, 0xfd, 0x17, 0xfe, 0xfd, 0x00, 0x01,
			     0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

int lowpan_ghc_decompression(struct sk_buff *skb, size_t needed, struct ipv6hdr *hdr)
{
	/* IPv6 src addr (16 Byte)  + IPv6 dst addr (16 Byte) + static dict (16 Byte)
	 * + reconstituted payload (max 127 byte */
	u8 buf[48 + 127];
	u8 codebyte = 0;
	int i;
	int buf_idx;
	int sa = 0;
	int na = 0;
	bool fail;
	int len = skb->len;
	struct in6_addr *src_addr = &hdr->saddr;
	struct in6_addr *dst_addr = &hdr->daddr;

	/* Fill the beginning of the output buffer with the given IP addresses
	 * and static dictionary */
	memcpy(&buf[0], src_addr->s6_addr, 16);
	memcpy(&buf[16], dst_addr->s6_addr, 16);
	memcpy(&buf[32], &static_dict[0], 16);
	buf_idx = 47;

	for (i = 0; i < len; i++)  {
		fail = lowpan_fetch_skb(skb, &codebyte, sizeof(codebyte));
		pr_debug("Codebyte %x at index %i\n", codebyte, i);

		/* 10010000: stop code (end of compressed data), optional */
		if (codebyte ==  GHC_STOP_CODE) {
			pr_debug("Found stop code\n");
			break;
		}

		/* 0kkkkkkk: append k = 0b0kkkkkkk bytes of data in the bytecode
		 * argument (k < 96) (with k bytes as argument) */
		if ((codebyte >> 7) == 0x00) {
			int idx;
			pr_debug("Found append code, appending %i next bytes to buffer\n", codebyte);
			/* FIXME: fetch in one go instead of loop */
			for (idx = 1; idx < codebyte + 1; idx++) {
				//buf[buf_idx] = compressed[i + idx];
				lowpan_fetch_skb(skb, &buf[buf_idx], 1);
				buf_idx++;
			}
			i += codebyte;
			//printf("Index = %i\n", i);
			continue;
		}

		/* 1000nnnn: append 0b0000nnnn+2 bytes of zeroes */
		if ((codebyte >> 4) == 0x08) {
			int idx;
			pr_debug("Found zeros append code, appending %i zeros\n", (codebyte & 0x0f) +2);
			for (idx = 0; idx < ((codebyte & 0x0f) +2); idx++) {
				buf[buf_idx] = 0x00;
				buf_idx++;
			}
			//printf("Index = %i\n", i);
			continue;
		}

		/* 101nssss: set up extended arguments for a backreference:
		 * sa += 0b0ssss000, na += 0b0000n000 */
		if ((codebyte >> 5) == 0x05) {
			sa += ((codebyte & 0x0f) << 3);
			na += ((codebyte & 0x10) >> 1) ;
			pr_debug("Found setup extended arguments code, sa = %i, na = %i\n", sa, na);
			//printf("Index = %i\n", i);
			continue;
		}

		/* 11nnnkkk: Backreference: n = na+0b00000nnn+2;
		 * s = 0b00000kkk+sa+n; append n bytes from previously output
		 * bytes, starting s bytes to the left of the current output
		 * pointer; set sa = 0, na = 0 */
		if ((codebyte >> 6) == 0x03) {
			int idx;
			int nnn = (codebyte & 0x38) >> 3;
			int kkk = codebyte & 0x07;
			int n = na + nnn + 2;
			int s = sa + kkk + n;

			pr_debug("Found backreference code, params: na = %i, sa = %i, n = %i, s = %i, nnn = %i, kkk = %i\n", na, sa, n, s, nnn, kkk);
			for (idx = 0; idx < n; idx++) {
				buf[buf_idx] = buf[buf_idx - s];
				buf_idx++;
			}
			na = 0;
			sa = 0;
			//printf("Index = %i\n", i);
			continue;
		}
	}
	/* FIXME: write decompressed payload (buf > 47) back into skb */

	return 0;
}
//EXPORT_SYMBOL(lowpan_ghc_decompression);
