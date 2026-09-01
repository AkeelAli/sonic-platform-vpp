# VPP Port Drop Stats HLD

## Table of Contents

1. [Revisions](#revisions)
2. [Scope](#scope)
3. [Problem Statement](#problem-statement)
4. [Solution](#solution)
5. [Design Details](#design-details)
6. [Code References](#code-references)
7. [Expected Behavior](#expected-behavior)
8. [Appendix](#appendix)

## Revisions

| Rev | Date | Author(s) |
|-----|------|-----------|
| v0.1 | 08/31/2026 | Akeel Ali |

## Scope

This document describes the high level design for counting interface drops
against the original physical ingress port on the SONiC VPP platform.

The change is implemented as a VPP dataplane patch,
`vppbld/patches/0008-drop-stats-track-original-member-interface.patch`, applied
to the bundled VPP source used by the `syncd-vpp` image. It ensures that a drop
is also counted on the physical member port's `drops` counter when the RX
interface index has been rewritten by LAG (bonding) or SVI/BVI forwarding. It is
purely a VPP dataplane change: there are no SAI, orchagent, SONiC CLI, or
database-schema changes, and interfaces whose RX index is not rewritten are
unaffected.

## Problem Statement

VPP's `error-drop` node runs `interface_drop_punt()`, which increments the
per-interface `drops` counter (`/interfaces/<if>/drops`,
`VNET_INTERFACE_COUNTER_DROP`) for the packet's RX interface index
`sw_if_index[VLIB_RX]`. sairedis maps this `drops` counter to
`SAI_PORT_STAT_IF_IN_DISCARDS` (RX_DRP in `show interfaces counters`).

Two forwarding features in VPP rewrite the RX interface index
`sw_if_index[VLIB_RX]` before a packet can reach the drop path:

1. **LAG / bonding** — `bond_sw_if_idx_rewrite()` replaces the member's RX index
   with the bond interface's index.
2. **SVI / VLAN routing** — `l2_to_bvi()` replaces the RX index with the index of
   the bridge domain's BVI (Bridge Virtual Interface) to route out of the bridge
   domain.

As a result, a drop on a LAG member or a VLAN member is counted
against the LAG or BVI, not the physical member, so the ingress port shows zero
drops. This gap is exercised by the sonic-mgmt drop-counter tests
(`tests/drop_packets/test_drop_counters.py`), whose tests consequently fail when
parametrized over `port_channel_members` and `vlan_members` in the `t1-lag-vpp`
and `t0-vpp` topologies.

To address this gap, the original port must be captured at the rewrite and
carried on the packet buffer to the drop node, where the counter increment can
then be applied against the original physical member port in addition to the
rewritten LAG or BVI interface.

## Solution

Capture the original RX interface index on the buffer at each rewrite site, mark
it valid with a dedicated buffer flag, and have `interface_drop_punt()` add a
second counter increment against it.

Validity is signaled by a buffer flag rather than a sentinel value because
`b->flags` is reset on every buffer allocation (copied from the pool template)
while `opaque2` is not. The flag therefore provides correct per-packet validity
at near-zero fast-path cost, with no extra initialization node, and it composes
correctly with buffer clones used for flood/replication.

## Design Details

### Buffer storage and validity flag (`src/vnet/buffer.h`)

- A `u32 orig_rx_sw_if_index` field is carved from the tail `unused[]` words of
  `vnet_buffer_opaque2_t`, so the struct does not grow:

  ```c
  u32 orig_rx_sw_if_index;
  u32 unused[5];
  ```

- Buffer flag bit 27 (previously `AVAIL9`) is repurposed as
  `VNET_BUFFER_F_ORIG_RX_SW_IF_VALID` and removed from
  `VNET_BUFFER_FLAGS_ALL_AVAIL`. The highest available bit is chosen to minimize
  rebase collisions with upstream, which allocates from the lowest free bit.

### Capture at the LAG rewrite (`src/vnet/bonding/node.c`)

In both branches of `bond_sw_if_idx_rewrite()`, before the RX index is
overwritten with the bond index:

```c
b->flags |= VNET_BUFFER_F_ORIG_RX_SW_IF_VALID;
vnet_buffer2 (b)->orig_rx_sw_if_index = vnet_buffer (b)->sw_if_index[VLIB_RX];
```

### Capture at the BVI rewrite (`src/vnet/l2/l2_bvi.h`)

In `l2_to_bvi()`, record the ingress member before the BVI rewrite, but only if
nothing has recorded it yet, so a bonded packet keeps its physical member instead
of the LAG:

```c
if (!(b0->flags & VNET_BUFFER_F_ORIG_RX_SW_IF_VALID))
  {
    b0->flags |= VNET_BUFFER_F_ORIG_RX_SW_IF_VALID;
    vnet_buffer2 (b0)->orig_rx_sw_if_index =
      vnet_buffer (b0)->sw_if_index[VLIB_RX];
  }
```

### Second increment at drop (`src/vnet/interface_output.c`)

In `interface_drop_punt()`, after the existing LAG/BVI interface increment, walk the
buffers just counted and, gated on the flag, increment the original member port:

```c
u32 orig_off = frame->n_vectors - n_left - count;
for (u32 j = 0; j < count; j++)
  {
    vlib_buffer_t *ob = bufs[orig_off + j];
    if (!(ob->flags & VNET_BUFFER_F_ORIG_RX_SW_IF_VALID))
      continue;
    u32 orig = vnet_buffer2 (ob)->orig_rx_sw_if_index;
    if (orig != sw_if_index[0])
      vlib_increment_simple_counter (cm, thread_index, orig, 1);
  }
```

The flag check prevents reading a stale `opaque2` word, and the
`orig != sw_if_index[0]` check avoids double-counting when the original equals the
interface already counted.

`interface_drop_punt()` is shared with the `error-punt` node, so this same `cm`
increment also bumps the member's `punt` counter; because sairedis does not export
`punt` to a SAI port stat, only the `drops` / in-discards case is user-visible.

## Code References

The implementation is the VPP dataplane patch
`vppbld/patches/0008-drop-stats-track-original-member-interface.patch`.

| Area | File (patched VPP source) |
|------|---------------------------|
| Buffer field + validity flag | `src/vnet/buffer.h` |
| Capture at LAG member ingress | `src/vnet/bonding/node.c` (`bond_sw_if_idx_rewrite`) |
| Capture at SVI/BVI rewrite | `src/vnet/l2/l2_bvi.h` (`l2_to_bvi`) |
| Second drop increment | `src/vnet/interface_output.c` (`interface_drop_punt`) |

## Expected Behavior

1. A drop on a LAG member port is counted on both the LAG and the physical
   member port.
2. A drop on a VLAN member port is counted on both the BVI and the physical
   member port.
3. For a `member -> LAG -> BVI` chain, the drop is counted against the physical
   member, not the intermediate LAG.
4. Traffic on ports whose RX index is not rewritten is counted exactly as before.
5. The drop appears on the physical member's `SAI_PORT_STAT_IF_IN_DISCARDS`,
   satisfying the `port_channel_members` case (`t1-lag-vpp`) via the LAG capture
   and the `vlan_members` case (`t0-vpp`) via the SVI/BVI capture in
   `tests/drop_packets/test_drop_counters.py`; the `rif_members` case is
   unaffected.

## Appendix

A previous version of this patch only stored the index in `opaque2` and treated
`0` as "not set", zeroing it in `bond_input`. That was fragile: `opaque2` is not
reset per packet, so packets that skip `bond_input` could read a stale non-zero
value. It was also costlier on the fast path: since `opaque2` is not reset per
buffer allocation, the sentinel had to be written on every packet to establish a
known "not set" state (and to be fully correct would have needed a reset on every
path reaching the drop node, not just `bond_input`). The current design (buffer
flag plus a dedicated `opaque2` word) addresses those issues — `b->flags` is
already reset on every buffer allocation, so per-packet validity comes for free.

This approach however has the disadvantage that it relies on non-upstream buffer
state — a scarce per-buffer flag bit (bit 27, formerly `AVAIL9`) plus a `u32` in
`opaque2` — so it is fragile across VPP rebases. It permanently consumes one of
VPP's limited `AVAIL` flag bits and must be re-checked on every rebase:
if upstream later assigns bit 27, the patch has to move to another free bit.
The flag also guards only against stale reads across packets, not against another
node overwriting the `orig_rx_sw_if_index` word in `opaque2` for the same packet
between the RX rewrite and the drop; that word is carved from the tail `unused[]`
words that nothing touches today, so that case is low risk.
