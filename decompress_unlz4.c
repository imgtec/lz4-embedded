/*
 * a very simple lz4 decoder
 * Copyright (c) 2016 Du Huanpeng<u74147@gmail.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int __lz4cpy(char *dst, const char *src, int n)
{
	int i = 0;
	int j = 0;

	unsigned int token;
	unsigned int len;
	int error = 0;

#define limit len	/* alias */

	fprintf(stderr, "info: decoding...\n");

	do {
		token = (unsigned char)src[i++];
		len = 15U & (token>>4);
		if(len == 15U) {
			do {
				len += (unsigned int)src[i];
			} while(src[i++] == '\255');
		}
		limit = i + len;
		while(i<limit) {
			dst[j] = src[i];
			j++, i++;
		}
		len = 15U & token;
		
#define offset token	/* alias */
		offset  = (unsigned char)src[i++];
		offset += (unsigned char)src[i++]*256U;

		if(len == 15U) {
			do {
				len += (unsigned int)src[i];
			} while(src[i++] == '\255');
		}
		len += 4;
		limit = j + len;
		if(limit>=n) {
			error |= 0x80000001;
			fprintf(stderr, "warning: dest buffer overflow[0x%08x]\n", limit);
			limit = n;
		}
		if(j<offset){
			fprintf(stderr, "bug-bug!: invail offset[%08x]\n", offset);
			error |= 0x80000002;
			return error;
		}

		while(j<limit) {
			dst[j] = dst[j-offset];
			j++;
		}
		if(j == n) {
			return n;
		}
		if(i+32U>65535) { /* TODO: finish */
			fprintf(stderr, "info: finish %s", error ? "with error\n" : "\n");
			error = j;
		}
	} while(error == 0);
	return error;
}

/*
 *
 * ref: https://github.com/lz4/lz4/blob/dev/doc/lz4_Frame_format.md
 */
void *lz4cpy(void *dst, const void *src, int n)
{
	const char *p;
	int error;

	p = src;

	if((p[0] != '\x04')
		|| (p[1] != '\x22')
		|| (p[2] != '\x4D')
		|| (p[3] != '\x18'))
		return memcpy(dst, src, n);
	p += 4;

	if(*p & '\x8') p += 8;
	else p += 3;

	p++;

	src = (char *)src + 12;

	error = __lz4cpy(dst, src, n);
	if(error>0) return dst;
	else return NULL;
}



#ifdef DEBUG
char lz4file[] = {
	#include "test.lz4"
};

char original[] = {
	#include "test.bin"
};

char decoded[sizeof(original)+64];

int main(int argc, char *argv[])
{
	int i, error;

	lz4cpy(original, lz4file, sizeof(original));
	for (int i = 0; i < sizeof(original); ++i) {
		if(original[i] != decoded[i]) error++;
	}
	printf("error: %d\n", error);

	return 0;
}
#endif
