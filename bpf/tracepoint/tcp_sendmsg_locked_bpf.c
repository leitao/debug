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

struct event_data {
    long pid;
    long ts;
    const void *skb_addr;
    int skb_len;
    int msg_left;
    int size_goal;
};


SEC("tracepoint/tcp/tcp_sendmsg_locked")
int bpf_tcp_sendmsg_locked(struct tcp_sendmsg_locked_args *ctx)
{
    struct event_data data = {};
    
    // Get process ID
    data.pid = bpf_get_current_pid_tgid() >> 32;
    
    // Get timestamp
    data.ts = bpf_ktime_get_ns();
    
    // Copy tracepoint data
    data.skb_addr = ctx->skb_addr;
    data.skb_len = ctx->skb_len;
    data.msg_left = ctx->msg_left;
    data.size_goal = ctx->size_goal;
    
    // Log to trace pipe for debugging
    bpf_printk("TCP: skb_addr %p skb_len %d msg_left %d size_goal %d",
               data.skb_addr, data.skb_len, data.msg_left, data.size_goal);
    
    return 0;
}




char LICENSE[] SEC("license") = "GPL";
