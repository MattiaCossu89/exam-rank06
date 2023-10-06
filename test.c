#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
	char *buf = malloc(1024 * 256);
	int count = 0, loops = 100, sum = 0;
	clock_t begin, end;
  	clock_t begin1, end1;

	for (int i = 0; i < 1024 * 256; ++i)
		buf[i] = '0';
	bzero(buf + 1024  * 256 - 3, 2);
	buf[1024 * 256 - 1] = 0;
	
	
	for (int i = 0; i < loops; ++i) {
		begin = clock();
		strlen(buf);
		end = clock();
		usleep(10000);
		begin1 = clock();
		strlcpy(buf, buf + 2, 1024 * 256 - 3);
		end1 = clock();
		if (end - begin < end1 - begin1) {
			++count;
			sum += end + end1 - begin - begin1;
		}
	}
	printf("strlen: %f\nstrcpy: %f\nstrlen faster: %d\nsprintf faster: %d\naverage difference:%d\n",
	 (double)(end - begin), (double)(end1 - begin1), count, loops - count, sum / 1001);
}
