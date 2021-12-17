#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <inttypes.h>
#define PAGE_SIZE 0x1000

#define FIND_LIB_NAME


static uint64_t get_page_flags(int fd, uint64_t pfn) {
	uint64_t data;
        if(pread(fd, &data, sizeof(data), pfn * sizeof(data)) != sizeof(data)) {
            if(errno) perror("pread");
	    exit(1);
	}
	return data;
}

static uint64_t get_page_count(int fd, uint64_t pfn) {
	uint64_t data;
        if(pread(fd, &data, sizeof(data), pfn * sizeof(data)) != sizeof(data)) {
            if(errno) perror("pread");
	    exit(1);
	}
	return data;
}

static void print_page(int fd_count, int fd_flag, uint64_t address, uint64_t data,
    const char *lib_name) {
    uint64_t pfn = data & 0x7fffffffffffff;
    printf("0x%-16"PRIx64" : pfn 0x%-16"PRIx64" page-count %-16"PRIu64" page-flag 0x%-16"PRIx64" soft-dirty %"PRIu64" file/shared %"PRIu64
        "swapped %"PRIu64" present %"PRIu64"library %s\n",
        address,
        pfn,
	get_page_count(fd_count, pfn),
	get_page_flags(fd_flag, pfn),
        (data >> 55) & 1,
        (data >> 61) & 1,
        (data >> 62) & 1,
        (data >> 63) & 1,
        lib_name);
}

void handle_virtual_range(int pagemap, int fd_count, int fd_flag, uint64_t start_address,
    uint64_t end_address, const char *lib_name) {

    for(uint64_t i = start_address; i < end_address; i += 0x1000) {
        uint64_t data;
        uint64_t index = (i / PAGE_SIZE) * sizeof(data);
        if(pread(pagemap, &data, sizeof(data), index) != sizeof(data)) {
            if(errno) perror("pread");
            break;
        }
	
        print_page(fd_count, fd_flag, i, data, lib_name);
    }
}

void parse_maps(const char *maps_file, const char *pagemap_file) {
    int maps = open(maps_file, O_RDONLY);
    if(maps < 0) return;

    int pagemap = open(pagemap_file, O_RDONLY);
    if(pagemap < 0) {
        close(maps);
        return;
    }
    int fd_count = open("/proc/kpagecount", O_RDONLY);
    if(fd_count < 0) {
        close(maps);
        close(pagemap);
        return;
    }
    int fd_flag = open("/proc/kpageflags", O_RDONLY);
    if(fd_flag < 0) {
        close(maps);
        close(pagemap);
        close(fd_count);
        return;
    }

    char buffer[BUFSIZ];
    int offset = 0;

    for(;;) {
        ssize_t length = read(maps, buffer + offset, sizeof buffer - offset);
        if(length <= 0) break;

        length += offset;

        for(size_t i = offset; i < (size_t)length; i ++) {
            uint64_t low = 0, high = 0;
            if(buffer[i] == '\n' && i) {
                size_t x = i - 1;
                while(x && buffer[x] != '\n') x --;
                if(buffer[x] == '\n') x ++;

                while(buffer[x] != '-' && x+1 < sizeof buffer) {
                    char c = buffer[x ++];
                    low *= 16;
                    if(c >= '0' && c <= '9') {
                        low += c - '0';
                    }
                    else if(c >= 'a' && c <= 'f') {
                        low += c - 'a' + 10;
                    }
                    else break;
                }

                while(buffer[x] != '-' && x+1 < sizeof buffer) x ++;
                if(buffer[x] == '-') x ++;

                while(buffer[x] != ' ' && x+1 < sizeof buffer) {
                    char c = buffer[x ++];
                    high *= 16;
                    if(c >= '0' && c <= '9') {
                        high += c - '0';
                    }
                    else if(c >= 'a' && c <= 'f') {
                        high += c - 'a' + 10;
                    }
                    else break;
                }

                const char *lib_name = 0;
#ifdef FIND_LIB_NAME
                for(int field = 0; field < 4; field ++) {
                    x ++;  // skip space
                    while(buffer[x] != ' ' && x+1 < sizeof buffer) x ++;
                }
                while(buffer[x] == ' ' && x+1 < sizeof buffer) x ++;

                size_t y = x;
                while(buffer[y] != '\n' && y+1 < sizeof buffer) y ++;
                buffer[y] = 0;

                lib_name = buffer + x;
#endif

                handle_virtual_range(pagemap, fd_count, fd_flag, low, high, lib_name);

#ifdef FIND_LIB_NAME
                buffer[y] = '\n';
#endif
            }
        }
    }

    close(maps);
    close(pagemap);
    close(fd_count);
    close(fd_flag);
}


void process_pid(pid_t pid) {
    char maps_file[BUFSIZ];
    char pagemap_file[BUFSIZ];
    snprintf(maps_file, sizeof(maps_file),
        "/proc/%"PRIu64"/maps", (uint64_t)pid);
    snprintf(pagemap_file, sizeof(pagemap_file),
        "/proc/%"PRIu64"/pagemap", (uint64_t)pid);

    parse_maps(maps_file, pagemap_file);
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: %s pid1 [pid2...]\n", argv[0]);
        return 1;
    }

    for(int i = 1; i < argc; i ++) {
        pid_t pid = (pid_t)strtoul(argv[i], NULL, 0);

        printf("=== Maps for pid %d\n", (int)pid);
        process_pid(pid);
    }

    return 0;
}
