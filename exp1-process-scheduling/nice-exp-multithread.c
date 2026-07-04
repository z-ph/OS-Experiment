#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

void *thread_fun(void *param)
{
    pid_t tid = syscall(SYS_gettid);
    FILE *fp = fopen("/tmp/nice-exp.tid", "w");
    if (fp) {
        fprintf(fp, "%d\n", tid);
        fclose(fp);
    }
    while (1)
        ;
    return NULL;
}

int main(void)
{
    pthread_t tid;
    int ret;
    ret = pthread_create(&tid, NULL, thread_fun, NULL);
    if (ret != 0) {
        perror("cannot create new thread");
        return 1;
    }
    if (pthread_join(tid, NULL) != 0) {
        perror("call pthread_join function fail");
        return 1;
    }
    return 0;
}
