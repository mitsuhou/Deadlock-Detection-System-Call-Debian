#include "deadlock.h"

bool is_in_array(pid_t pid, pid_t *arr, int count) {
    int i;
    for (i = 0; i < count; i++) {
        if (arr[i] == pid) return true;
    }
    return false;
}

bool dfs(pid_t u, pid_t *visited, int *v_count, pid_t *rec_stack, int *rs_count) {
    struct resource_node *res;
    struct waiter_node *w;
    
    visited[(*v_count)++] = u;
    rec_stack[(*rs_count)++] = u;

    list_for_each_entry(res, &resource_list, list) {
        bool is_u_waiting = false;
        
        list_for_each_entry(w, &res->waiters_list, list) {
            if (w->pid == u) {
                is_u_waiting = true;
                break;
            }
        }

        if (is_u_waiting && res->owner != 0) {
            pid_t v = res->owner;
            
            if (!is_in_array(v, visited, *v_count)) {
                if (dfs(v, visited, v_count, rec_stack, rs_count))
                    return true;
            } else if (is_in_array(v, rec_stack, *rs_count)) {
                return true; 
            }
        }
    }
    
    (*rs_count)--;
    return false;
}
