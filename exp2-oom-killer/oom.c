#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STEP_SIZE (4 * 1024 * 1024)

int main(void)
{
    long count = 0;
    for (;;) {
        char *memory = (char *)malloc(STEP_SIZE);
        if (!memory) {
            perror("malloc");
            sleep(1);
            continue;
        }
        memset(memory, 0, STEP_SIZE);
        count++;
        if (count % 25 == 0) {
            printf("allocated about %ld MB\n", count * 4);
        }
        sleep(1);
    }
    return 0;
}
