/*
 * a very simple lz4 decoder
 * Copyright (c) 2016 Du Huanpeng<u74147@gmail.com>
 *
 */


extern void *lz4cpy(void *dst, const void *src, int n);

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	char *ib;
	char *ob;
	int i, j;
	int ch;
	FILE *is, *os;

	ib = malloc(16*1024*1024);
	ob = malloc(16*1024*1024);
	if(ib==NULL || ob==NULL) {
		exit(-1);
	}

	if(argc <3) {
		exit(-2);
	}

	is = fopen(argv[1], "rb");
	os = fopen(argv[2], "wb");
	if(is==NULL || os==NULL) {
		exit(-3);
	}
	/* lets go */
	i = 0; j = 0;
	while((ch=fgetc(is))!=EOF){
		ib[i]=ch;
		if(j==0) i++;
		else j=i;	/* tricky drain */
	}

	lz4cpy(ib, ob, 0xFFFFFFFF);

	for(i=0; i<j; i++){
		putchar(ob[i]);
	}
	return 0;
}
