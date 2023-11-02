#!/usr/local/bin/bcc-py
from bcc import BPF
from time import sleep
import argparse
import sys
import ctypes as ct
import traceback
import time
import os



bpf_source = """
#include <linux/types.h>
#include <linux/cgroup.h>



struct val {
	u64 refcnt;
	u64 gets;
	u64 puts;
	u64 done;
	u32 stack_id[100];
	u32 idx;
};


BPF_STACK_TRACE(stack_traces, 100000);
BPF_HASH(hmap, u64, struct val, 100);


static void update(struct cgroup_subsys_state *css, s64 v, u64 stack_id)
{
        u64 cgid = css->cgroup->kn->id;
    	struct val zero = {};
    	struct val *cur;
        // Only care about cgroup core
        if (css->ss != NULL) {
                return;
        }
        // Only care about cgroups created after tracing began
        cur = hmap.lookup(&cgid);
        if (!cur) {
                return;
        }
        /* Get the refcont for the cgid */
        cur->refcnt = atomic64_read(&css->refcnt.data->count);
	if (v == 0) {
		return;
	}
	if (v > 0) {
		cur->gets += v;
	} else {
		cur->puts += -v;
	}

	if (cur->idx < 100) {
	 	cur->stack_id[cur->idx] = stack_id;
	 	cur->idx += 1;
	}
        hmap.update(&cgid, cur);
}

KRETFUNC_PROBE(psi_cgroup_alloc, struct cgroup *cgrp)
{
        struct val zero = { 0 };
        u64 cgid = cgrp->kn->id;
        hmap.insert(&cgid, &zero);
        return 0;
}
KRETFUNC_PROBE(css_get, struct cgroup_subsys_state *css)
{
	u64 id = stack_traces.get_stackid(ctx, 0);
        update(css, 1, id);
        return 0;
}
KRETFUNC_PROBE(css_get_many, struct cgroup_subsys_state *css, unsigned int n)
{
	u64 id = stack_traces.get_stackid(ctx, 0);
        update(css, n, id);
        return 0;
}
KRETFUNC_PROBE(css_tryget, struct cgroup_subsys_state *css, int ret)
{
	u64 id = stack_traces.get_stackid(ctx, 0);
        if (ret)
            update(css, 1, id);
        return 0;
}
KRETFUNC_PROBE(css_tryget_online, struct cgroup_subsys_state *css, int ret)
{
	u64 id = stack_traces.get_stackid(ctx, 0);
        if (ret)
            update(css, 1, id);
        return 0;
}
KRETFUNC_PROBE(css_put, struct cgroup_subsys_state *css)
{
	u64 id = stack_traces.get_stackid(ctx, 0);
        update(css, -1, id);
        return 0;
}
KRETFUNC_PROBE(css_put_many, struct cgroup_subsys_state *css, unsigned int n)
{
	u64 id = stack_traces.get_stackid(ctx, 0);
        update(css, -n, id);
        return 0;
}

KFUNC_PROBE(css_release_work_fn, struct work_struct *work)
{
        struct cgroup_subsys_state *css = container_of(work, struct cgroup_subsys_state, destroy_work);
        struct cgroup_subsys *ss;
        struct cgroup *cgrp;
        struct kernfs_node *kn;
        u64 cgid;
    	struct val *cur;


        bpf_probe_read_kernel(&ss, sizeof(ss), &css->ss);
        if (ss)
                return 0;
        bpf_probe_read_kernel(&cgrp, sizeof(cgrp), &css->cgroup);
        bpf_probe_read_kernel(&kn, sizeof(kn), &cgrp->kn);
        bpf_probe_read_kernel(&cgid, sizeof(cgid), &kn->id);

        cur = hmap.lookup(&cgid);
        if (!cur) {
                return 0;
        }
	cur->done = 1;

        hmap.update(&cgid, cur);

        return 0;
}


"""
def do_list():
	for entry, value in hmap.items():
		if value.done:
			# Discarding cgroups that were done
			continue
		print(f"{entry.value} : refcnt={hex(value.refcnt)}, get={value.gets}, put={value.puts} done={value.done}")

def dump_stack_nr(i):
	print("----------")
	try:
		for addr in stack.walk(i):
			try:
				sym = bpf.ksym(addr).decode("utf-8")
				if sym.startswith("bpf"):
					continue
				off = addr - bpf.ksymname(sym)
				print(f'  {sym}+{off}')
			except:
				print(f'  0x{addr:x}', file=file)
	except:
		print('  BACKTRACE FAILED', file=file)


def get_stack_put_or_get(i):
	for addr in stack.walk(i):
		sym = bpf.ksym(addr).decode("utf-8")
		if sym.startswith("bpf"):
			continue
		# First stack
		return sym
	return ""



def do_count(cid):
	for entry, value in hmap.items():
		sget = 0
		sput = 0
		none = 0
		if entry.value != int(cid):
			continue

		print(f"refcnt = {value.refcnt}")
		print(f"gets = {value.gets}")
		print(f"puts = {value.puts}")
		for i in value.stack_id:
			if i != 0:
				line = get_stack_put_or_get(i)
				if "get" in line:
					sget += 1
				elif "put" in line:
					sput += 1
				else:
					none += 1
		print(f"stack ending in get = {sget}")
		print(f"stack ending in puts = {sput}")
		print(f"stack failures = {none}")


def do_dump(cid):
	print(f"Dumping {cid}")
	for entry, value in hmap.items():
		if entry.value != int(cid):
			continue

		print(f"refcnt = {value.refcnt}")
		print(f"gets = {value.gets}")
		print(f"puts = {value.puts}")
		print(f"idx = {value.idx}")
		for i in value.stack_id:
			if i != 0:
				dump_stack_nr(i)
		#print(f"{value.stack_id}")



# Starts here
bpf = BPF(text=bpf_source)
hmap = bpf['hmap']
stack = bpf['stack_traces']

while True:
	cmd = input('COMMAND: ').split()
	if cmd[0] == 'list' or cmd[0] == 'l':
            do_list()
	if cmd[0] == 'dump' or cmd[0] == 'd':
            do_dump(cmd[1])
	if cmd[0] == 'quit' or cmd[0] == 'q':
	    os.exit()
	if cmd[0] == 'count' or cmd[0] == 'c':
	    do_count(cmd[1])
