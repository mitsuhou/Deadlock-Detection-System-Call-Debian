#include "deadlock.h"

LIST_HEAD(resource_list);
DEFINE_MUTEX(resource_mutex);

SYSCALL_DEFINE1(register_resource, int, resource_id)
{
    struct resource_node *res, *tmp;

    mutex_lock(&resource_mutex);

    list_for_each_entry(tmp, &resource_list, list) {
        if (tmp->id == resource_id) {
            mutex_unlock(&resource_mutex);
            return -EEXIST;
        }
    }

    res = kmalloc(sizeof(struct resource_node), GFP_KERNEL);
    if (!res) {
        mutex_unlock(&resource_mutex);
        return -ENOMEM;
    }

    res->id = resource_id;
    res->owner = 0;
    init_waitqueue_head(&res->wait_queue);
    INIT_LIST_HEAD(&res->waiters_list);
    
    list_add_tail(&res->list, &resource_list);

    mutex_unlock(&resource_mutex);
    return 0;

SYSCALL_DEFINE1(request_resource, int, resource_id)
{
    struct resource_node *res = NULL, *tmp;
    struct waiter_node *waiter;
    DEFINE_WAIT(wait);

    mutex_lock(&resource_mutex);

    list_for_each_entry(tmp, &resource_list, list) {
        if (tmp->id == resource_id) {
            res = tmp;
            break;
        }
    }

    if (!res) {
        mutex_unlock(&resource_mutex);
        return -ENOENT;
    }

    if (res->owner == 0) {
        res->owner = current->pid;
        mutex_unlock(&resource_mutex);
        return 0;
    }

    if (res->owner == current->pid) {
        mutex_unlock(&resource_mutex);
        return 0;
    }

    waiter = kmalloc(sizeof(*waiter), GFP_KERNEL);
    if (!waiter) {
        mutex_unlock(&resource_mutex);
        return -ENOMEM;
    }
    waiter->pid = current->pid;
    list_add_tail(&waiter->list, &res->waiters_list);

    while (1) {
        prepare_to_wait(&res->wait_queue, &wait, TASK_INTERRUPTIBLE);
        
        if (res->owner == 0) {
            res->owner = current->pid; // Take ownership!
            break;
        }

        if (signal_pending(current)) {
            list_del(&waiter->list);
            kfree(waiter);
            finish_wait(&res->wait_queue, &wait);
            mutex_unlock(&resource_mutex);
            return -EINTR;
        }

        mutex_unlock(&resource_mutex);
        
        schedule(); 
        
        mutex_lock(&resource_mutex);
    }

    finish_wait(&res->wait_queue, &wait);
    
    list_del(&waiter->list);
    kfree(waiter);

    mutex_unlock(&resource_mutex);
    return 0;
}

SYSCALL_DEFINE1(release_resource, int, resource_id)
{
    struct resource_node *res = NULL, *tmp;

    mutex_lock(&resource_mutex);

    list_for_each_entry(tmp, &resource_list, list) {
        if (tmp->id == resource_id) {
            res = tmp;
            break;
        }
    }

    if (!res) {
        mutex_unlock(&resource_mutex);
        return -ENOENT;
    }

    if (res->owner != current->pid) {
        mutex_unlock(&resource_mutex);
        return -EPERM;
    }

    res->owner = 0; 

    wake_up_all(&res->wait_queue);

    mutex_unlock(&resource_mutex);
    return 0;
}

SYSCALL_DEFINE0(detect_deadlock)
{
    pid_t visited[MAX_PROCS];
    pid_t rec_stack[MAX_PROCS];
    int v_count = 0;
    int cycle_found = 0;
    
    pid_t unique_procs[MAX_PROCS];
    int p_count = 0;
    int i;

    struct resource_node *res;
    struct waiter_node *w;

    mutex_lock(&resource_mutex);

    list_for_each_entry(res, &resource_list, list) {
        if (res->owner != 0 && !is_in_array(res->owner, unique_procs, p_count)) {
            if (p_count < MAX_PROCS) unique_procs[p_count++] = res->owner;
        }
        list_for_each_entry(w, &res->waiters_list, list) {
            if (!is_in_array(w->pid, unique_procs, p_count)) {
                if (p_count < MAX_PROCS) unique_procs[p_count++] = w->pid;
            }
        }
    }

    for (i = 0; i < p_count; i++) {
        if (!is_in_array(unique_procs[i], visited, v_count)) {
            int rs_count = 0;
            if (dfs(unique_procs[i], visited, &v_count, rec_stack, &rs_count)) {
                cycle_found = 1;
                break;
            }
        }
    }

    mutex_unlock(&resource_mutex);
    return cycle_found;
}
