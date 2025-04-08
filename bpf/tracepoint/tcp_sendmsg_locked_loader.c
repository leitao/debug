#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

static volatile bool exiting = false;

void sig_handler(int sig)
{
    exiting = true;
}

int main(int argc, char **argv)
{
    struct bpf_object *obj;
    int err;
    struct bpf_link *link;

    /* Open the BPF object file */
    obj = bpf_object__open("tcp_sendmsg_locked_bpf.o");
    if (!obj) {
        fprintf(stderr, "Failed to open BPF object file\n");
        return 1;
    }

    /* Load the BPF program */
    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        bpf_object__close(obj);
        return 1;
    }

    /* Find the BPF program */
    struct bpf_program *prog = bpf_object__find_program_by_name(obj, "tcp_sendmsg_locked");
    if (!prog) {
        fprintf(stderr, "Failed to find BPF program\n");
        bpf_object__close(obj);
        return 1;
    }

    /* Attach the BPF program to the tracepoint */
    link = bpf_program__attach(prog);
    if (!link) {
        fprintf(stderr, "Failed to attach BPF program: %d\n", err);
        bpf_object__close(obj);
        return 1;
    }

    /* Set up signal handler */
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("BPF program attached to tcp_sendmsg_locked tracepoint. Press Ctrl+C to exit.\n");
    printf("Run 'sudo cat /sys/kernel/debug/tracing/trace_pipe' to see the output.\n");

    /* Keep the program running until interrupted */
    while (!exiting) {
        sleep(1);
    }

    /* Cleanup */
    bpf_object__close(obj);
    printf("BPF program detached.\n");

    return 0;
}
