#ifndef UTILS_H
#define UTILS_H

#pragma once

#include <time.h>
#include <channel_cnf.h>

/* options, parameters */
struct Args {
	unsigned int flags;		// verbose on 0x1   B25 on 0x1000
	unsigned int recsec;
	unsigned int splitter;
	unsigned int ts_id;
	char sid_list[32];
	char* devfile;
	char* destfile;
	CHANNEL_INFO channel_info;
};

int parseOption(int argc, char * const argv[], struct Args *);
void setSignalHandler(const int mode, void (*func_handler)(int));

#define u_gettime(tp)  clock_gettime(CLOCK_MONOTONIC, tp)
void u_difftime(struct timespec *, struct timespec *, int *, int *);

struct OutputBuffer {
	void *buffer;
	struct OutputBuffer* pOutput;
	int bufSize;
	int length;
	int (* process)(struct OutputBuffer* const, void* const buf);
	int (* release)(struct OutputBuffer* const);
};

extern int OutputBuffer_release(struct OutputBuffer* const);
extern int OutputBuffer_put(struct OutputBuffer* const, void *buf, unsigned length);
extern int OutputBuffer_flush(struct OutputBuffer* const);
extern struct OutputBuffer* create_FileBufferedWriter(unsigned  bufSize, const char* const);
extern struct OutputBuffer* create_TSParser(unsigned  bufSize, struct OutputBuffer* const  pOutput, const unsigned  mode, void *usbdev);

#endif

/*EOF*/
