#!/usr/local/bin/bcc-py
from bcc import BPF
from time import sleep
import argparse
import sys
import ctypes as ct
import traceback
import time

bpf_source = """
#include <linux/types.h>
#include <linux/cgroup.h>

BPF_HASH(hmap, u64, u64, 100);

static void update(struct cgroup_subsys_state *css, s64 v)
{
        u64 cgid = css->cgroup->kn->id;

        if (!css->ss){
                return;
        }

        /* Get the refcont for the cgid */
        u64 z = atomic64_read(&css->refcnt.data->count);

        u64 *count = hmap.lookup_or_try_init(&cgid, &z);
        if (!count) {
            return;
        }
        hmap.update(&cgid, count);

}

KFUNC_PROBE(css_get, struct cgroup_subsys_state *css)
{
        update(css, 1);
        return 0;
}

KFUNC_PROBE(css_get_many, struct cgroup_subsys_state *css, unsigned int n)
{
        update(css, n);
        return 0;
}

KRETFUNC_PROBE(css_tryget, struct cgroup_subsys_state *css, int ret)
{
        if (ret)
            update(css, 1);
        return 0;
}

KRETFUNC_PROBE(css_tryget_online, struct cgroup_subsys_state *css, int ret)
{
        if (ret)
            update(css, 1);
        return 0;
}

KFUNC_PROBE(css_put, struct cgroup_subsys_state *css)
{
        update(css, -1);
        return 0;
}

KFUNC_PROBE(css_put_many, struct cgroup_subsys_state *css, unsigned int n)
{
        update(css, -n);
        return 0;
}

"""

bpf = BPF(text=bpf_source)
hmap = bpf['hmap']
time.sleep(1)

for entry, value in hmap.items():
    print(f"{entry.value} -> {hex(value.value)}")
