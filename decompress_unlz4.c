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
#ifdef DEBUG
	unsigned int limit;
	unsigned int offset;
#else
	#define limit len
	#define offset token
#endif

	fprintf(stderr, "info: decoding...\n");

	do {
		token = (unsigned char)src[i++];
		len = 15U & (token>>4);
		if(len == 15U) {
			do {
				len += (unsigned char)src[i];
			} while(src[i++] == '\255');
		}
		limit = i + len;
/* len */	while(i<limit) {
			dst[j] = src[i];
			j++, i++;
		}
		len = 15U & token;
		
/* token */	offset  = (unsigned char)src[i++];
		offset += (unsigned char)src[i++]*256U;

		if(len == 15U) {
			do {
				len += (unsigned char)src[i];
			} while(src[i++] == '\255');
		}
		len += 4;
/* len */		limit = j + len;
		if(limit>=n) {
			error |= 0x80000001;
			fprintf(stderr, "warning: dest buffer overflow[0x%08x]\n", limit);
			limit = n;
		}
		if(offset>j){
			fprintf(stderr, "bug!: invail offset[%08x]\n", offset);
			error |= 0x80000002;
			#ifdef DEBUG
			goto dump;
			#endif
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
#ifdef DEBUG
dump:
	fprintf(stderr, "variable dump\n\n");
	fprintf(stderr, " i(in): [%8x]\n", i);
	fprintf(stderr, "j(out): [%8x]\n", j);
	fprintf(stderr, " token: [  %02X]\n", token);
	fprintf(stderr, "offset: [%04X]\n", offset);
	fprintf(stderr, "   len: [%04X]\n", len);
	fprintf(stderr, " limit: [%4d]\n", limit);
	fprintf(stderr, "\n");
	return error;
#else
	#undef offset
	#undef limit
#endif

}

/*
 *
 * ref: https://github.com/lz4/lz4/blob/dev/doc/lz4_Frame_format.md
 */
void *lz4cpy(void *dst, const void *src, int n)
{
	const char *p;
	int error;

	if(n<13)
		return memcpy(dst, src, n);

	p = src;

	if((p[0] != '\x04')
		|| (p[1] != '\x22')
		|| (p[2] != '\x4D')
		|| (p[3] != '\x18'))
		return memcpy(dst, src, n);
	p += 4;

	if(*p & '\x8') p += 8;
	else p += 3;

	//src = (char *)src + 7;
	//src = (char *)src + 15;
	src = (char *)src + 11;
	//src = (void *)p;

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

	memset(decoded, 0, sizeof(decoded));

	lz4cpy(decoded, lz4file, sizeof(original));
	for (i = 0, error = 0; i < sizeof(original); i++) {
		if(!error) putchar(decoded[i]);
		if(original[i] != decoded[i]) error++;
	}
	printf("error: %d\n", error);

	return 0;
}
#endif
