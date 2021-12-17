#include<stdio.h>
#include<sys/mman.h>
#include<stdint.h>
#include<unistd.h>
#include <errno.h>

#define MEM_SIZE  0x4000000
int main()
{
        void *addr;
        int index, ret;
        pid_t pid = getpid();
        printf("PID = %d\n", pid);
        addr = mmap(NULL, MEM_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
        if (MAP_FAILED == addr) {
                printf("mmap failed\n");
                return 1;
        }
        printf("mmap success: addr = %p, size = 0x%x\n", addr, MEM_SIZE);
        for (index = 0; index < MEM_SIZE; index += 8) {
                *((uint64_t *)(addr + index)) = 0xff55ff55ff55ff00;
        }
        getchar();
        printf("start to lock memory\n");
        ret = mlock(addr, MEM_SIZE);
        if(ret) {
                perror("mlock failed");
		goto err;
        }
        ret=getchar();
	munlock(addr, MEM_SIZE);
        printf("start to unlock memory\n");
        ret=getchar();
err:
        return munmap(addr, MEM_SIZE);
}
