#ifndef DEADLOCK_H
#define DEADLOCK_H

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/wait.h>

#define MAX_PROCS 100

struct waiter_node {
    pid_t pid;
    struct list_head list;
};

struct resource_node {
    int id;
    pid_t owner;
    wait_queue_head_t wait_queue;
    struct list_head waiters_list;
    struct list_head list;
};

extern struct list_head resource_list;
extern struct mutex resource_mutex;

bool is_in_array(pid_t pid, pid_t *arr, int count);
bool dfs(pid_t u, pid_t *visited, int *v_count, pid_t *rec_stack, int *rs_count);

#endif
