#!/usr/local/bin/bcc-py
from bcc import BPF
from time import sleep
import argparse
import sys
import ctypes as ct
import traceback
parser = argparse.ArgumentParser(formatter_class = argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-e', '--ents', type=int, default=4096,
                    help='Maximum number of entities to track (default 4096)')
parser.add_argument('-s', '--stacks', type=int, default=4096,
                    help='Maximum number of stack IDs to track (default 1024)')
bpf_source = """
#include <linux/types.h>
#include <linux/cgroup.h>
/**
 * MEMBER_VPTR - Obtain the verified pointer to a struct or array member
 * @base: struct or array to index
 * @member: dereferenced member (e.g. ->field, [idx0][idx1], ...)
 *
 * The verifier often gets confused by the instruction sequence the compiler
 * generates for indexing struct fields or arrays. This macro forces the
 * compiler to generate a code sequence which first calculates the byte offset,
 * checks it against the struct or array size and add that byte offset to
 * generate the pointer to the member to help the verifier.
 *
 * This works because the compiler is prevented from optimizing across the
 * volatile store/load pair. The byte offset is only available in that variable
 * and that variable can be read only once, so there's no other way that the
 * compiler can generate that offset, and because it's a byte offset, there's no
 * shifting or other operations which can confuse the verifier during
 * dereference.
 *
 * Ideally, we want to abort if the calculated offset is out-of-bounds. However,
 * BPF currently doesn't support abort, so evaluate to NULL instead. The caller
 * must check for NULL and take appropriate action to appease the verifier. To
 * avoid confusing the verifier, it's best to check for NULL and dereference
 * immediately.
 *
 *	vptr = MEMBER_VPTR(my_array, [i][j]);
 *	if (!vptr)
 *		return error;
 *	*vptr = new_value;
 */
#define MEMBER_VPTR(array, idxs) ({						\
	u64 __v, __off;								\
	*(volatile u64 *)&__v =							\
		(unsigned long)&(array idxs) - (unsigned long)array;		\
	__off = *(volatile u64 *)&__v;						\
	(typeof(array idxs) *)(__off < sizeof(array) ?				\
			       ((unsigned long)array + __off) : 0);		\
})
BPF_STACK_TRACE(stack_traces, __NR_STACKS__);
struct ref_hist {
        bool            dying;
        u64             cgrp_ptr;
        s32             cnts[__NR_STACKS__];
};
BPF_HASH(ref_hist, u64, struct ref_hist, __NR_ENTS__);
BPF_ARRAY(ref_hist_zero, struct ref_hist, 1);

BPF_HASH(hmap, u64, long, 100);

static void track_ref(struct cgroup_subsys_state *css, s64 v, u64 stack_id)
{
        u32 zero_idx = 0;
        u64 cgid = css->cgroup->kn->id;
        struct ref_hist *zero, *hist;
        struct ref_hist *ref_errors;
        struct ref_rec *rec;
        u64 idx;
        u32 *cnt;
        long z = 0 ;
        if (!css->ss){
                return;
        }
        zero = ref_hist_zero.lookup(&zero_idx);
        if (!zero) {
            bpf_trace_printk("no zero");
            return;
        }
        zero->cgrp_ptr = (u64)css->cgroup;

        /* Get the refcont for the cgid */
        z = css->refcnt.percpu_count_ptr;

        unsigned long *count = hmap.lookup_or_try_init(&cgid, &z);
        if (!count) {
            return;
        }
        hmap.update(&cgid, count);

        hist = ref_hist.lookup_or_try_init(&cgid, zero);
        if (!hist) {
                bpf_trace_printk("No hist");
                return;
        }
        cnt = MEMBER_VPTR(hist->cnts, [stack_id]);
        if (cnt)
                __sync_fetch_and_add(cnt, v);
        hist->dying = css->refcnt.percpu_count_ptr & __PERCPU_REF_DEAD;
}
KFUNC_PROBE(css_get, struct cgroup_subsys_state *css)
{
        track_ref(css, 1, stack_traces.get_stackid(ctx, 0));
        return 0;
}
KFUNC_PROBE(css_get_many, struct cgroup_subsys_state *css, unsigned int n)
{
        track_ref(css, n, stack_traces.get_stackid(ctx, 0));
        return 0;
}
KRETFUNC_PROBE(css_tryget, struct cgroup_subsys_state *css, int ret)
{
        if (ret)
                track_ref(css, 1, stack_traces.get_stackid(ctx, 0));
        return 0;
}
KRETFUNC_PROBE(css_tryget_online, struct cgroup_subsys_state *css, int ret)
{
        if (ret)
                track_ref(css, 1, stack_traces.get_stackid(ctx, 0));
        return 0;
}
KFUNC_PROBE(css_put, struct cgroup_subsys_state *css)
{
        track_ref(css, -1, stack_traces.get_stackid(ctx, 0));
        return 0;
}
KFUNC_PROBE(css_put_many, struct cgroup_subsys_state *css, unsigned int n)
{
        track_ref(css, -n, stack_traces.get_stackid(ctx, 0));
        return 0;
}
KFUNC_PROBE(css_release_work_fn, struct work_struct *work)
{
        struct cgroup_subsys_state *css = container_of(work, struct cgroup_subsys_state, destroy_work);
        struct cgroup_subsys *ss;
        struct cgroup *cgrp;
        struct kernfs_node *kn;
        u64 cgid;
        bpf_probe_read_kernel(&ss, sizeof(ss), &css->ss);
        if (ss)
                return 0;
        bpf_probe_read_kernel(&cgrp, sizeof(cgrp), &css->cgroup);
        bpf_probe_read_kernel(&kn, sizeof(kn), &cgrp->kn);
        bpf_probe_read_kernel(&cgid, sizeof(cgid), &kn->id);
        ref_hist.delete(&cgid);
        return 0;
}
"""
args = parser.parse_args()
bpf_source = bpf_source.replace('__NR_ENTS__', str(args.ents))
bpf_source = bpf_source.replace('__NR_STACKS__', str(args.stacks))
bpf = BPF(text=bpf_source)
stack_traces = bpf['stack_traces']
ref_hist = bpf['ref_hist']

hmap = bpf['hmap']
def format_uptime(at_ms):
    at = at_ms / 1000
    return f'{int(at / 3600)}:{int((at % 3600) / 60)}:{at % 60:.3f}'

def report_one(cgid, file=sys.stdout):
    hist = ref_hist[ct.c_uint64(cgid)]
    print(f'==== cgroup {cgid} ptr {hex(hist.cgrp_ptr)} ====', file=file)
    for i in range(args.stacks):
        if hist.cnts[i] == 0:
            continue
        print(f'\nCOUNT={hist.cnts[i]}', file=file)
        try:
            for addr in stack_traces.walk(i):
                try:
                    sym = bpf.ksym(addr)
                    off = addr - bpf.ksymname(sym)
                    print(f'  {sym.decode("utf-8")}+{off}', file=file)
                except:
                    print(f'  0x{addr:x}', file=file)
        except:
            print('  BACKTRACE FAILED', file=file)

def do_list():
    for cgid, hist in ref_hist.items():
        if not hist.dying:
            continue
        total = 0;
        for i in range(args.stacks):
            total += hist.cnts[i];
        print(f'[{cgid.value}] total={total} cgrp_ptr={hex(hist.cgrp_ptr)}')
help_msg = """list             : List dying cgroups with non-zero ref
hist CGID        : Dump reference history of the cgroup
quit             : Exit the program
help             : Show this help message"""
print('"help" to view the help message"')


# MAIN
while True:
    try:
        cmd = input('COMMAND: ').split()
        if len(cmd) == 0:
            continue;
        if cmd[0] == 'help':
            print(help_msg)
        if cmd[0] == 'e':
            for entry, value in hmap.items():
                print(f"{entry.value} -> {value.value}")
        elif cmd[0] == 'list':
            do_list()
        elif cmd[0] == 'hist':
            cgid = int(cmd[1], 0)
            if len(cmd) >= 3:
                with open(cmd[2], 'w') as out:
                    report_one(cgid, file=out)
            else:
                report_one(cgid)
        elif cmd[0] == 'quit':
            sys.exit(0)
        else:
            print(f'Unknown command "{cmd}"')
    except KeyboardInterrupt:
        print('Use "quit" to exit')
    except EOFError:
        print('Use "quit" to exit')
    except Exception:
        traceback.print_exc()

