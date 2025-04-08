#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>


struct tcp_sendmsg_locked_args {
    long __unused__;  // First 8 bytes are padding
    const void *skb_addr;
    int skb_len;
    int msg_left;
    int size_goal;
};


SEC("tracepoint/tcp/tcp_sendmsg_locked")
int bpf_tcp_sendmsg_locked(struct tcp_sendmsg_locked_args *ctx)
{
    // Log to trace pipe for debugging
    bpf_printk("TCP: skb_addr %p skb_len %d msg_left %d size_goal %d",
               ctx->skb_addr, ctx->skb_len, ctx->msg_left, ctx->size_goal);
    
    /* bpf_printk("deref %d\n", *(int *) ctx->skb_addr); */
    return 0;
}




char LICENSE[] SEC("license") = "GPL";
