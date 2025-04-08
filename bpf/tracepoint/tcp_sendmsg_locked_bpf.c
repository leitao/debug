#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>


SEC("tp_btf/tcp_sendmsg_locked")
int BPF_PROG(tcp_sendmsg_locked, struct sock *sk, struct msghdr *msg, struct sk_buff *skb, int size_goal)
{
	bpf_printk("skb->len %d\n", skb->len);

	return 0;
}

char LICENSE[] SEC("license") = "GPL";
