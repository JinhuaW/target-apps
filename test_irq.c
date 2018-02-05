#include <libuio.h>
#include <sys/select.h>
#include <stdio.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

int main()
{
	fd_set read_set;
	int fd, ret;
	unsigned long intrs;
	struct uio_info_t *uio = uio_find_by_uio_name("test_irq");
	ret = uio_open(uio);
	assert(ret == 0);
	fd = uio_get_fd(uio);
	assert(fd >= 0);
	while (true) {
		FD_SET(fd, &read_set);
		do {
			ret = select(fd + 1, &read_set, NULL, NULL, NULL);
		} while ((ret < 0) && (errno == EINTR));
		if (ret < 0)
			return -1;
		if (FD_ISSET(fd, &read_set)) {
			read(fd, &intrs, sizeof(intrs)); 
			printf("handled %lu irqs.\n", intrs);
		}
	}
	uio_close(uio);
	return 0;
}
