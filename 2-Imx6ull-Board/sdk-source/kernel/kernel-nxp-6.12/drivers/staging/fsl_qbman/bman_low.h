/* Copyright 2008-2011 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Freescale Semiconductor nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "bman_private.h"

/***************************/
/* Portal register assists */
/***************************/

/* Cache-inhibited register offsets */
#define BM_REG_RCR_PI_CINH	0x3000
#define BM_REG_RCR_CI_CINH	0x3100
#define BM_REG_RCR_ITR		0x3200
#define BM_REG_CFG		0x3300
#define BM_REG_SCN(n)		(0x3400 + ((n) << 6))
#define BM_REG_ISR		0x3e00
#define BM_REG_IIR              0x3ec0

/* Cache-enabled register offsets */
#define BM_CL_CR		0x0000
#define BM_CL_RR0		0x0100
#define BM_CL_RR1		0x0140
#define BM_CL_RCR		0x1000
#define BM_CL_RCR_PI_CENA	0x3000
#define BM_CL_RCR_CI_CENA	0x3100

/* BTW, the drivers (and h/w programming model) already obtain the required
 * synchronisation for portal accesses via lwsync(), hwsync(), and
 * data-dependencies. Use of barrier()s or other order-preserving primitives
 * simply degrade performance. Hence the use of the __raw_*() interfaces, which
 * simply ensure that the compiler treats the portal registers as volatile (ie.
 * non-coherent). */

/* Cache-inhibited register access. */
#define __bm_in(bm, o)		be32_to_cpu(__raw_readl((bm)->addr_ci + (o)))
#define __bm_out(bm, o, val)    __raw_writel(cpu_to_be32(val), \
					     (bm)->addr_ci + (o));
#define bm_in(reg)		__bm_in(&portal->addr, BM_REG_##reg)
#define bm_out(reg, val)	__bm_out(&portal->addr, BM_REG_##reg, val)

/* Cache-enabled (index) register access */
#define __bm_cl_touch_ro(bm, o) dcbt_ro((bm)->addr_ce + (o))
#define __bm_cl_touch_rw(bm, o) dcbt_rw((bm)->addr_ce + (o))
#define __bm_cl_in(bm, o)	be32_to_cpu(__raw_readl((bm)->addr_ce + (o)))
#define __bm_cl_out(bm, o, val) \
	do { \
		u32 *__tmpclout = (bm)->addr_ce + (o); \
		__raw_writel(cpu_to_be32(val), __tmpclout); \
		dcbf(__tmpclout); \
	} while (0)
#define __bm_cl_invalidate(bm, o) dcbi((bm)->addr_ce + (o))
#define bm_cl_touch_ro(reg) __bm_cl_touch_ro(&portal->addr, BM_CL_##reg##_CENA)
#define bm_cl_touch_rw(reg) __bm_cl_touch_rw(&portal->addr, BM_CL_##reg##_CENA)
#define bm_cl_in(reg)	    __bm_cl_in(&portal->addr, BM_CL_##reg##_CENA)
#define bm_cl_out(reg, val) __bm_cl_out(&portal->addr, BM_CL_##reg##_CENA, val)
#define bm_cl_invalidate(reg)\
	__bm_cl_invalidate(&portal->addr, BM_CL_##reg##_CENA)

/* Cyclic helper for rings. FIXME: once we are able to do fine-grain perf
 * analysis, look at using the "extra" bit in the ring index registers to avoid
 * cyclic issues. */
static inline u8 bm_cyc_diff(u8 ringsize, u8 first, u8 last)
{
	/* 'first' is included, 'last' is excluded */
	if (first <= last)
		return last - first;
	return ringsize + last - first;
}

/* Portal modes.
 *   Enum types;
 *     pmode == production mode
 *     cmode == consumption mode,
 *   Enum values use 3 letter codes. First letter matches the portal mode,
 *   remaining two letters indicate;
 *     ci == cache-inhibited portal register
 *     ce == cache-enabled portal register
 *     vb == in-band valid-bit (cache-enabled)
 */
enum bm_rcr_pmode {		/* matches BCSP_CFG::RPM */
	bm_rcr_pci = 0,		/* PI index, cache-inhibited */
	bm_rcr_pce = 1,		/* PI index, cache-enabled */
	bm_rcr_pvb = 2		/* valid-bit */
};
enum bm_rcr_cmode {		/* s/w-only */
	bm_rcr_cci,		/* CI index, cache-inhibited */
	bm_rcr_cce		/* CI index, cache-enabled */
};


/* ------------------------- */
/* --- Portal structures --- */

#define BM_RCR_SIZE		8

struct bm_rcr {
	struct bm_rcr_entry *ring, *cursor;
	u8 ci, available, ithresh, vbit;
#ifdef CONFIG_FSL_DPA_CHECKING
	u32 busy;
	enum bm_rcr_pmode pmode;
	enum bm_rcr_cmode cmode;
#endif
};

struct bm_mc {
	struct bm_mc_command *cr;
	struct bm_mc_result *rr;
	u8 rridx, vbit;
#ifdef CONFIG_FSL_DPA_CHECKING
	enum {
		/* Can only be _mc_start()ed */
		mc_idle,
		/* Can only be _mc_commit()ed or _mc_abort()ed */
		mc_user,
		/* Can only be _mc_retry()ed */
		mc_hw
	} state;
#endif
};

struct bm_addr {
	void __iomem *addr_ce;	/* cache-enabled */
	void __iomem *addr_ci;	/* cache-inhibited */
};

struct bm_portal {
	struct bm_addr addr;
	struct bm_rcr rcr;
	struct bm_mc mc;
	struct bm_portal_config config;
} ____cacheline_aligned;


/* --------------- */
/* --- RCR API --- */

/* Bit-wise logic to wrap a ring pointer by clearing the "carry bit" */
#define RCR_CARRYCLEAR(p) \
	(void *)((unsigned long)(p) & (~(unsigned long)(BM_RCR_SIZE << 6)))

/* Bit-wise logic to convert a ring pointer to a ring index */
static inline u8 RCR_PTR2IDX(struct bm_rcr_entry *e)
{
	return ((uintptr_t)e >> 6) & (BM_RCR_SIZE - 1);
}

/* Increment the 'cursor' ring pointer, taking 'vbit' into account */
static inline void RCR_INC(struct bm_rcr *rcr)
{
	/* NB: this is odd-looking, but experiments show that it generates
	 * fast code with essentially no branching overheads. We increment to
	 * the next RCR pointer and handle overflow and 'vbit'. */
	struct bm_rcr_entry *partial = rcr->cursor + 1;
	rcr->cursor = RCR_CARRYCLEAR(partial);
	if (partial != rcr->cursor)
		rcr->vbit ^= BM_RCR_VERB_VBIT;
}

static inline int bm_rcr_init(struct bm_portal *portal, enum bm_rcr_pmode pmode,
		__maybe_unused enum bm_rcr_cmode cmode)
{
	/* This use of 'register', as well as all other occurrences, is because
	 * it has been observed to generate much faster code with gcc than is
	 * otherwise the case. */
	register struct bm_rcr *rcr = &portal->rcr;
	u32 cfg;
	u8 pi;

	rcr->ring = portal->addr.addr_ce + BM_CL_RCR;
	rcr->ci = bm_in(RCR_CI_CINH) & (BM_RCR_SIZE - 1);

	pi = bm_in(RCR_PI_CINH) & (BM_RCR_SIZE - 1);
	rcr->cursor = rcr->ring + pi;
	rcr->vbit = (bm_in(RCR_PI_CINH) & BM_RCR_SIZE) ?  BM_RCR_VERB_VBIT : 0;
	rcr->available = BM_RCR_SIZE - 1
		- bm_cyc_diff(BM_RCR_SIZE, rcr->ci, pi);
	rcr->ithresh = bm_in(RCR_ITR);
#ifdef CONFIG_FSL_DPA_CHECKING
	rcr->busy = 0;
	rcr->pmode = pmode;
	rcr->cmode = cmode;
#endif
	cfg = (bm_in(CFG) & 0xffffffe0) | (pmode & 0x3); /* BCSP_CFG::RPM */
	bm_out(CFG, cfg);
	return 0;
}

static inline void bm_rcr_finish(struct bm_portal *portal)
{
	register struct bm_rcr *rcr = &portal->rcr;
	u8 pi = bm_in(RCR_PI_CINH) & (BM_RCR_SIZE - 1);
	u8 ci = bm_in(RCR_CI_CINH) & (BM_RCR_SIZE - 1);
	DPA_ASSERT(!rcr->busy);
	if (pi != RCR_PTR2IDX(rcr->cursor))
		pr_crit("losing uncommited RCR entries\n");
	if (ci != rcr->ci)
		pr_crit("missing existing RCR completions\n");
	if (rcr->ci != RCR_PTR2IDX(rcr->cursor))
		pr_crit("RCR destroyed unquiesced\n");
}

static inline struct bm_rcr_entry *bm_rcr_start(struct bm_portal *portal)
{
	register struct bm_rcr *rcr = &portal->rcr;
	DPA_ASSERT(!rcr->busy);
	if (!rcr->available)
		return NULL;
#ifdef CONFIG_FSL_DPA_CHECKING
	rcr->busy = 1;
#endif
	return rcr->cursor;
}

static inline void bm_rcr_abort(struct bm_portal *portal)
{
	__maybe_unused register struct bm_rcr *rcr = &portal->rcr;
	DPA_ASSERT(rcr->busy);
#ifdef CONFIG_FSL_DPA_CHECKING
	rcr->busy = 0;
#endif
}

static inline struct bm_rcr_entry *bm_rcr_pend_and_next(
					struct bm_portal *portal, u8 myverb)
{
	register struct bm_rcr *rcr = &portal->rcr;
	DPA_ASSERT(rcr->busy);
	DPA_ASSERT(rcr->pmode != bm_rcr_pvb);
	if (rcr->available == 1)
		return NULL;
	rcr->cursor->__dont_write_directly__verb = myverb | rcr->vbit;
	dcbf_64(rcr->cursor);
	RCR_INC(rcr);
	rcr->available--;
	return rcr->cursor;
}

static inline void bm_rcr_pci_commit(struct bm_portal *portal, u8 myverb)
{
	register struct bm_rcr *rcr = &portal->rcr;
	DPA_ASSERT(rcr->busy);
	DPA_ASSERT(rcr->pmode == bm_rcr_pci);
	rcr->cursor->__dont_write_directly__verb = myverb | rcr->vbit;
	RCR_INC(rcr);
	rcr->available--;
	hwsync();
	bm_out(RCR_PI_CINH, RCR_PTR2IDX(rcr->cursor));
#ifdef CONFIG_FSL_DPA_CHECKING
	rcr->busy = 0;
#endif
}

static inline void bm_rcr_pce_prefetch(struct bm_portal *portal)
{
	__maybe_unused register struct bm_rcr *rcr = &portal->rcr;
	DPA_ASSERT(rcr->pmode == bm_rcr_pce);
	bm_cl_invalidate(RCR_PI);
	bm_cl_touch_rw(RCR_PI);
}

static inline void bm_rcr_pce_commit(struct bm_portal *portal, u8 myverb)
{
	register struct bm_rcr *rcr = &portal->rcr;
	DPA_ASSERT(rcr->busy);
	DPA_ASSERT(rcr->pmode == bm_rcr_pce);
	rcr->cursor->__dont_write_directly__verb = myverb | rcr->vbit;
	RCR_INC(rcr);
	rcr->available--;
	lwsync();
	bm_cl_out(RCR_PI, RCR_PTR2IDX(rcr->cursor));
#ifdef CONFIG_FSL_DPA_CHECKING
	rcr->busy = 0;
#endif
}

static inline void bm_rcr_pvb_commit(struct bm_portal *portal, u8 myverb)
{
	register struct bm_rcr *rcr = &portal->rcr;
	struct bm_rcr_entry *rcursor;
	DPA_ASSERT(rcr->busy);
	DPA_ASSERT(rcr->pmode == bm_rcr_pvb);
	lwsync();
	rcursor = rcr->cursor;
	rcursor->__dont_write_directly__verb = myverb | rcr->vbit;
	dcbf_64(rcursor);
	RCR_INC(rcr);
	rcr->available--;
#ifdef CONFIG_FSL_DPA_CHECKING
	rcr->busy = 0;
#endif
}

static inline u8 bm_rcr_cci_update(struct bm_portal *portal)
{
	register struct bm_rcr *rcr = &portal->rcr;
	u8 diff, old_ci = rcr->ci;
	DPA_ASSERT(rcr->cmode == bm_rcr_cci);
	rcr->ci = bm_in(RCR_CI_CINH) & (BM_RCR_SIZE - 1);
	diff = bm_cyc_diff(BM_RCR_SIZE, old_ci, rcr->ci);
	rcr->available += diff;
	return diff;
}

static inline void bm_rcr_cce_prefetch(struct bm_portal *portal)
{
	__maybe_unused register struct bm_rcr *rcr = &portal->rcr;
	DPA_ASSERT(rcr->cmode == bm_rcr_cce);
	bm_cl_touch_ro(RCR_CI);
}

static inline u8 bm_rcr_cce_update(struct bm_portal *portal)
{
	register struct bm_rcr *rcr = &portal->rcr;
	u8 diff, old_ci = rcr->ci;
	DPA_ASSERT(rcr->cmode == bm_rcr_cce);
	rcr->ci = bm_cl_in(RCR_CI) & (BM_RCR_SIZE - 1);
	bm_cl_invalidate(RCR_CI);
	diff = bm_cyc_diff(BM_RCR_SIZE, old_ci, rcr->ci);
	rcr->available += diff;
	return diff;
}

static inline u8 bm_rcr_get_ithresh(struct bm_portal *portal)
{
	register struct bm_rcr *rcr = &portal->rcr;
	return rcr->ithresh;
}

static inline void bm_rcr_set_ithresh(struct bm_portal *portal, u8 ithresh)
{
	register struct bm_rcr *rcr = &portal->rcr;
	rcr->ithresh = ithresh;
	bm_out(RCR_ITR, ithresh);
}

static inline u8 bm_rcr_get_avail(struct bm_portal *portal)
{
	register struct bm_rcr *rcr = &portal->rcr;
	return rcr->available;
}

static inline u8 bm_rcr_get_fill(struct bm_portal *portal)
{
	register struct bm_rcr *rcr = &portal->rcr;
	return BM_RCR_SIZE - 1 - rcr->available;
}


/* ------------------------------ */
/* --- Management command API --- */

static inline int bm_mc_init(struct bm_portal *portal)
{
	register struct bm_mc *mc = &portal->mc;
	mc->cr = portal->addr.addr_ce + BM_CL_CR;
	mc->rr = portal->addr.addr_ce + BM_CL_RR0;
	mc->rridx = (__raw_readb(&mc->cr->__dont_write_directly__verb) &
			BM_MCC_VERB_VBIT) ?  0 : 1;
	mc->vbit = mc->rridx ? BM_MCC_VERB_VBIT : 0;
#ifdef CONFIG_FSL_DPA_CHECKING
	mc->state = mc_idle;
#endif
	return 0;
}

static inline void bm_mc_finish(struct bm_portal *portal)
{
	__maybe_unused register struct bm_mc *mc = &portal->mc;
	DPA_ASSERT(mc->state == mc_idle);
#ifdef CONFIG_FSL_DPA_CHECKING
	if (mc->state != mc_idle)
		pr_crit("Losing incomplete MC command\n");
#endif
}

static inline struct bm_mc_command *bm_mc_start(struct bm_portal *portal)
{
	register struct bm_mc *mc = &portal->mc;
	DPA_ASSERT(mc->state == mc_idle);
#ifdef CONFIG_FSL_DPA_CHECKING
	mc->state = mc_user;
#endif
	return mc->cr;
}

static inline void bm_mc_abort(struct bm_portal *portal)
{
	__maybe_unused register struct bm_mc *mc = &portal->mc;
	DPA_ASSERT(mc->state == mc_user);
#ifdef CONFIG_FSL_DPA_CHECKING
	mc->state = mc_idle;
#endif
}

static inline void bm_mc_commit(struct bm_portal *portal, u8 myverb)
{
	register struct bm_mc *mc = &portal->mc;
	struct bm_mc_result *rr = mc->rr + mc->rridx;
	DPA_ASSERT(mc->state == mc_user);
	lwsync();
	mc->cr->__dont_write_directly__verb = myverb | mc->vbit;
	dcbf(mc->cr);
	dcbit_ro(rr);
#ifdef CONFIG_FSL_DPA_CHECKING
	mc->state = mc_hw;
#endif
}

static inline struct bm_mc_result *bm_mc_result(struct bm_portal *portal)
{
	register struct bm_mc *mc = &portal->mc;
	struct bm_mc_result *rr = mc->rr + mc->rridx;
	DPA_ASSERT(mc->state == mc_hw);
	/* The inactive response register's verb byte always returns zero until
	 * its command is submitted and completed. This includes the valid-bit,
	 * in case you were wondering... */
	if (!__raw_readb(&rr->verb)) {
		dcbit_ro(rr);
		return NULL;
	}
	mc->rridx ^= 1;
	mc->vbit ^= BM_MCC_VERB_VBIT;
#ifdef CONFIG_FSL_DPA_CHECKING
	mc->state = mc_idle;
#endif
	return rr;
}


/* ------------------------------------- */
/* --- Portal interrupt register API --- */

static inline int bm_isr_init(__always_unused struct bm_portal *portal)
{
	return 0;
}

static inline void bm_isr_finish(__always_unused struct bm_portal *portal)
{
}

#define SCN_REG(bpid) BM_REG_SCN((bpid) / 32)
#define SCN_BIT(bpid) (0x80000000 >> (bpid & 31))
static inline void bm_isr_bscn_mask(struct bm_portal *portal, u8 bpid,
					int enable)
{
	u32 val;
	DPA_ASSERT(bpid < bman_pool_max);
	/* REG_SCN for bpid=0..31, REG_SCN+4 for bpid=32..63 */
	val = __bm_in(&portal->addr, SCN_REG(bpid));
	if (enable)
		val |= SCN_BIT(bpid);
	else
		val &= ~SCN_BIT(bpid);
	__bm_out(&portal->addr, SCN_REG(bpid), val);
}

static inline u32 __bm_isr_read(struct bm_portal *portal, enum bm_isr_reg n)
{
#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
	return __bm_in(&portal->addr, BM_REG_ISR + (n << 6));
#else
	return __bm_in(&portal->addr, BM_REG_ISR + (n << 2));
#endif
}

static inline void __bm_isr_write(struct bm_portal *portal, enum bm_isr_reg n,
					u32 val)
{
#if defined(CONFIG_ARM) || defined(CONFIG_ARM64)
	__bm_out(&portal->addr, BM_REG_ISR + (n << 6), val);
#else
	__bm_out(&portal->addr, BM_REG_ISR + (n << 2), val);
#endif
}

/* Buffer Pool Cleanup */
static inline int bm_shutdown_pool(struct bm_portal *p, u32 bpid)
{
	struct bm_mc_command *bm_cmd;
	struct bm_mc_result *bm_res;
	bool stop = false;

	while (!stop) {
		/* Acquire buffers until empty */
		bm_cmd = bm_mc_start(p);
		bm_cmd->acquire.bpid = bpid;
		bm_mc_commit(p, BM_MCC_VERB_CMD_ACQUIRE |  1);
		while (!(bm_res = bm_mc_result(p)))
			cpu_relax();
		if (!(bm_res->verb & BM_MCR_VERB_ACQUIRE_BUFCOUNT)) {
			/* Pool is empty */
			/* TBD : Should we do a few extra iterations in
			   case some other some blocks keep buffers 'on deck',
			   which may also be problematic */
			stop = true;
		}
	}
	return 0;
}
