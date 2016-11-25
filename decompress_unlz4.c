/*
 * a very simple lz4 decoder
 * Copyright (c) 2016 Du Huanpeng<u74147@gmail.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
 * decode single block to @dst from @src
 */
int __lz4cpy(char *dst, const char *src, int blocksize)
{
	int i = 0;
	int j = 0;

	int token;
	int len;
	int error = 0;
#ifdef DEBUG
	int limit;
	int offset;
#else
	#define limit len
	#define offset token
#endif

	fprintf(stderr, "info: single block decoding...\n");

	fprintf(stderr, "info: block size[%08x]\n", blocksize);

	do {
		token = (unsigned char)src[i++];

		len = 15U & (token>>4);
		if(len == 15U) {
			do {
				len += (unsigned char)src[i];
			} while(src[i++] == '\255');
		}
		/* error now is zero, or will not reach here. */
		limit = j+len;
		if(limit+12>=blocksize) {
			error = blocksize;
			limit = blocksize;
		}
/* len */	while(j<limit) {
			dst[j] = src[i];
			j++, i++;
		}

		if(error) return j;

		len = 15U & token;
/* token */	offset  = (unsigned char)src[i++];
		/* LSL Rd, Rn, #8 */
		offset |= (unsigned char)src[i++]<<8U;

		if(len == 15U) {
			do {
				len += (unsigned char)src[i];
			} while(src[i++] == '\255');
		}
		len += 4;
		limit = j + len;
		/* check offset */
		if(offset>j){
			fprintf(stderr, "bug!: invail offset[%08x]\n", offset);
			error |= 0x82000000 | j;
			#ifdef DEBUG
			goto dump;
			#else
			return error;
			#endif
		}
		/* copy match */
		while(j<limit) {
			dst[j] = dst[j-offset];
			j++;
		}
		if(i>1234567) {
			fprintf(stderr, "info: unknow error[%08x]\n", i);
			error = j;
			error |= 0x88000000;
		}
	} while(error == 0);
	fprintf(stderr, "info: decode done[%08x]\n", error);
#ifdef DEBUG
dump:
	fprintf(stderr, "variable dump\n\n");
	fprintf(stderr, " block: [%8x]\n", blocksize);
	fprintf(stderr, " i(in): [%8x]\n", i);
	fprintf(stderr, "j(out): [%8x]\n", j);
	fprintf(stderr, " token: [------%02X]\n", token);
	fprintf(stderr, "offset: [----%04X]\n", offset);
	fprintf(stderr, "   len: [    %04X]\n", len);
	fprintf(stderr, " limit: [    %04X]\n", limit);
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
 *
 *
First of all, technically there is tricky part of the specification: Skippable Frames.But it seems you are new to LZ4 frame format, you can ignore it so far.You can revisit this part after you'll implement other parts.So please forget about Skippable frames.
If we can ignore skippable frame, first 6 bytes of the LZ4 frame format header always has same structure:
offset data +0 0x04 +1 0x22 +2 0x4d +3 0x18 +4 FLG of the Frame Descriptor +5 BD of the Frame Descriptor
And if bit 3 of FLG (1<<3 = 0x08, Content Size flag) is 0, header and the first block structure is
offset data +6 HC of the Frame Descriptor+7 bit [0,7] of the first block size +8 bit [8,15] of the first block size +9 bit [16,23] of the first block size +10 bit [24,31] of the first block size +11... data of the first block (compressed or uncompressed LZ4 raw block)...
And if bit 3 of FLG is 1 :
offset data +6 bit [0,7] of the Content Size +7 bit [8,15] of the Content Size +8 bit [16,23] of the Content Size +9 bit [24,31] of the Content Size +10 bit [32,39] of the Content Size +11 bit [40,47] of the Content Size +12 bit [48,55] of the Content Size +13 bit [56,63] of the Content Size +14 HC of the Frame Descriptor+15 bit [0,7] of the first block size +16 bit [8,15] of the first block size +17 bit [16,23] of the first block size +18 bit [24,31] of the first block size +19... data of the first block (compressed or uncompressed LZ4 raw block)...
 *
- - - - - - -
	[0-3]		04 22 4D 18
	[1]		FLG
	[1]		BD
	[1]		HC of the Frame Descriptor
	[3 or 11]	Content
- - - - - - -
	[4]		first block size
	[...]		data of the first block(s)
- - - - - - -
	[4]		block size
	[..]		block
- - - - - - -
	...		more blocks
- - - - - - -
	[4]		EndMark
	[0 or 4]	C. Checksum
- - - - - - -
	EOF
- - - - - - -
 */

void *lz4cpy(void *dst, const void *src, int n)
{
	const char *p;
	int error;
	int blocksize;

	if(n<13)
		return memcpy(dst, src, n);

	p = (char *)src;

	if((p[0] != '\x04') || (p[1] != '\x22') || (p[2] != '\x4D') || (p[3] != '\x18')) {
		fprintf(stderr, "magic not match [%02x %02x %02x %02x]\n",
			(unsigned char)p[0], (unsigned char)p[1],
			(unsigned char)p[2], (unsigned char)p[3]);
		return memcpy(dst, src, n);
	}

	p += 4;

	if(*p & '\x8') p += 8;
	else p += 3;

	/* BFI Rd, Rn, #lsl, #n */
	blocksize  = (unsigned char)p[3] << 0x18U;
	blocksize |= (unsigned char)p[2] << 0x10U;
	blocksize |= (unsigned char)p[1] << 0x08U;
	blocksize |= (unsigned char)p[0] << 0x00U;
	p+=4;

	src = (void *)p;

	error = __lz4cpy((char *)dst, (const char *)src, blocksize);
#ifdef DEBUG
	if((error & 0xFF000000) == 0) {
		fprintf(stderr, "info: decoded block size[  %06X]\n", error);
	} else {
		fprintf(stderr, "error: decode error[%08x]\n", error);
	}
#endif

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

char decoded[sizeof(original)];

int main(int argc, char *argv[])
{
	int i, error;

	memset(decoded, 0, sizeof(decoded));

	lz4cpy(decoded, lz4file, sizeof(original));
	for (i = 0, error = 0; i < sizeof(original); i++) {
		if(!error) putchar(decoded[i]);
		else break;
		if(original[i] != decoded[i]) error++;
	}
	printf("bytes error: %x\n", i);

	return 0;
}
#endif
