#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define SYS_register_resource 548
#define SYS_request_resource  549
#define SYS_release_resource  550
#define SYS_detect_deadlock   551

int main() {
    printf("=========================================\n");
    printf("   OS Deadlock Detection Demonstration   \n");
    printf("=========================================\n\n");
    fflush(stdout); 
    
    // 1. Đăng ký tài nguyên
    if (syscall(SYS_register_resource, 100) < 0) { perror("🚨 Lỗi Register R100"); exit(1); }
    if (syscall(SYS_register_resource, 200) < 0) { perror("🚨 Lỗi Register R200"); exit(1); }
    printf("[Monitor] Đã đăng ký Resource R100 và R200.\n\n");
    fflush(stdout);

    // 2. Process A
    pid_t pid1 = fork();
    if (pid1 == 0) {
        printf("[Process A:%d] Đang yêu cầu R100...\n", getpid());
        if (syscall(SYS_request_resource, 100) < 0) { perror("Lỗi Request R100"); exit(1); }
        printf("[Process A:%d] ĐÃ GIỮ R100! (Ngủ 3s chờ B)\n", getpid());
        
        sleep(3); // Ép chờ B lấy R200
        
        printf("[Process A:%d] Yêu cầu R200 (Sẽ bị Block tại đây...)\n", getpid());
        if (syscall(SYS_request_resource, 200) < 0) { perror("Lỗi Request R200"); exit(1); }
        exit(0);
    }

    // 3. Process B
    pid_t pid2 = fork();
    if (pid2 == 0) {
        printf("[Process B:%d] Đang yêu cầu R200...\n", getpid());
        if (syscall(SYS_request_resource, 200) < 0) { perror("Lỗi Request R200"); exit(1); }
        printf("[Process B:%d] ĐÃ GIỮ R200! (Ngủ 3s chờ A)\n", getpid());
        
        sleep(3); // Ép chờ A lấy R100
        
        printf("[Process B:%d] Yêu cầu R100 (Sẽ bị Block tại đây...)\n", getpid());
        if (syscall(SYS_request_resource, 100) < 0) { perror("Lỗi Request R100"); exit(1); }
        exit(0);
    }

    // 4. Monitor
    printf("[Monitor] Chờ 5 giây để ép Deadlock...\n\n");
    sleep(5); 

    printf("\n[Monitor] Gọi thuật toán sys_detect_deadlock()...\n");
    long is_deadlocked = syscall(SYS_detect_deadlock);
    
    if (is_deadlocked < 0) {
        perror("🚨 Lỗi chạy thuật toán DFS");
    } else if (is_deadlocked == 1) {
        printf("\n=======================================================\n");
        printf(" 🚨 KERNEL ĐÃ PHÁT HIỆN CHU TRÌNH: DEADLOCK CONFIRMED 🚨 \n");
        printf("=======================================================\n");
    } else {
        printf("[Monitor] HỆ THỐNG AN TOÀN (Không có Deadlock).\n");
    }

    // 5. Dọn dẹp
    kill(pid1, SIGKILL);
    kill(pid2, SIGKILL);
    wait(NULL); wait(NULL);
    return 0;
}