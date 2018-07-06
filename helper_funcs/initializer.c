
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#include "../src/Config.h"

typedef char* (*GeneratorFunc)(size_t, size_t*);

typedef struct
{
	const char* name;
	int (*driver)(const uint8_t*,size_t);
	GeneratorFunc generator;
} DRIVER_DESC_ID;

extern int (*DRIVER_PTR_ID)(const uint8_t*, size_t);

/* Zero terminated array */
extern const DRIVER_DESC_ID DRIVER_ARRAY_ID[];

static const char listOptionName[] = "--list-fuzz-drivers";
static const char optionName[] = "--fuzz-driver=";
static const char generatorOptionName[] = "--generate-for=";
static const size_t optionNameLen = sizeof(optionName) - 1;
static const size_t generatorOptionNameLen = sizeof(generatorOptionName) - 1;


void GeneratorUsage(char** argv)
{
	printf("Generator usage: %s %s<function> <outdir> <maxLen>\n", argv[0], generatorOptionName);
	exit(1);
}


int OpenValidatedDirectory(const char* dir)
{
	int fd = open(dir, O_DIRECTORY);
	if(fd < 0)
	{
		printf("Failed to open directory '%s': %m\n", dir);
		exit(1);
	}

	return fd;
}


void WriteToFile(int fd, const char* content, size_t contentLen)
{
	ssize_t remaining = contentLen;
	ssize_t ret;

	while(remaining > 0)
	{
		ret = write(fd, content, remaining);
		if(ret < 0)
		{
			printf("%p, %lu\n", content, remaining);
			printf("Write failed with error: %m\n");
			return;
		}

		content += ret;
		remaining -= ret;
	}
}

void GenerateInput(const char* dir, GeneratorFunc generator, size_t maxLen)
{
	int dirfd = OpenValidatedDirectory(dir);
	int fd;

	char nameBuf[64];

	char* buf;
	size_t bufLen;

	/* MaxSize */
	size_t lastLen = -1;
	for(size_t i = 0; i <= maxLen;)
	{
		buf = generator(i, &bufLen);

		/* When the size did not change the function includes no arrays */
		if(bufLen == lastLen)
		{
			free(buf);
			break;
		}

		snprintf(nameBuf, sizeof(nameBuf), "input_%lu", i);
		fd = openat(dirfd, nameBuf, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, S_IRUSR | S_IRUSR | S_IRGRP | S_IROTH);
		if(fd < 0)
		{
			printf("Couldn't create file in inputdir '%s': %m\n", nameBuf);
			if(i == 0)
				i = 1;
			else
				i *= 2;
			free(buf);
			continue;
		}

		lastLen = bufLen;
		WriteToFile(fd, buf, bufLen);
		free(buf);
		close(fd);

		if(i == 0)
			i = 1;
		else
			i *= 2;
	}
	exit(0);
}


int LLVMFuzzerInitialize(int* argc_ptr, char*** argv_ptr)
{
	int argc = *argc_ptr;
	char** argv = *argv_ptr;

	for(int i = 0; i < argc; ++i)
	{
		if(strncmp(argv[i], optionName, optionNameLen) == 0)
		{
			const char* requestedDriver = argv[i] + optionNameLen;
			for(const DRIVER_DESC_ID* fddesc = DRIVER_ARRAY_ID; fddesc->name; ++fddesc)
			{
				if(strcmp(fddesc->name, requestedDriver) == 0)
				{
					DRIVER_PTR_ID = fddesc->driver;
					break;
				}
			}
		}
		else if(strncmp(argv[i], generatorOptionName, generatorOptionNameLen) == 0)
		{
			if(i + 1 >= argc)
				GeneratorUsage(argv);

			size_t maxLen = 32;
			if(i + 2 < argc)
			{
				const char* arg = argv[i + 2];
				char* end;
				errno = 0;
				maxLen = strtoull(arg, &end, 10);
				if(errno || *end || !*arg)
				{
					printf("maxLen argument has invalid format (no number)\n");
					exit(1);
				}

			}

			const char* requestedGenerator = argv[i] + generatorOptionNameLen;
			GeneratorFunc generator = NULL;

			for(const DRIVER_DESC_ID* fddesc = DRIVER_ARRAY_ID; fddesc->name; ++fddesc)
			{
				if(strcmp(fddesc->name, requestedGenerator) == 0)
				{
					generator = fddesc->generator;
					break;
				}
			}

			if(!generator)
			{
				printf("Couldn't find generator for '%s'\n", requestedGenerator);
				exit(1);
			}
			GenerateInput(argv[i+1], generator, maxLen);
		}
		else if(strcmp(argv[i], listOptionName) == 0)
		{
			for(const DRIVER_DESC_ID* fddesc = DRIVER_ARRAY_ID; fddesc->name; ++fddesc)
				puts(fddesc->name);
			exit(0);
		}
	}
	if(DRIVER_PTR_ID == 0)
		exit(1);
	return 0;
}


/* Main for afl */
#ifdef __AFL_HAVE_MANUAL_CONTROL
int main(int argc, char** argv)
{
	/* Initialize driver to be used */
	LLVMFuzzerInitialize(&argc, &argv);

	/* Tell AFL to fork after the initialization */
	__AFL_INIT();

	/* Read from stdin into buffer */
	const size_t initialBufSize = 0x1000;
	size_t bufAlloc = initialBufSize;
	size_t bufUsage = 0;
	ssize_t bytesRead;
	char* buf = malloc(bufAlloc);

	while (__AFL_LOOP(1000))
	{
		while(1)
		{
			bytesRead = read(STDIN_FILENO, buf + bufUsage, bufAlloc - bufUsage);
			if(bytesRead < 0)
			{
				printf("read error...\n");
				exit(1);
			}
			if(bytesRead == 0)
				break;

			bufUsage += bytesRead;
			if(bufAlloc - bufUsage < 0x10)
			{
				size_t newAlloc = bufAlloc * 2;
				char* newBuf = realloc(buf, newAlloc);
				if(!newBuf)
					exit(1);
				buf = newBuf;
				bufAlloc = newAlloc;
			}
		}

		/* Give fuzzy input to driver */
		DRIVER_PTR_ID((const uint8_t*)buf, bufUsage);
		bufUsage = 0;
	}
	free(buf);

	return 0;
}
#endif


/* Main for calling test case */

#ifdef __REPRODUCE_FUZZING
int main(int argc, char** argv)
{
	/* Initialize driver to be used */
	LLVMFuzzerInitialize(&argc, &argv);

	/* Read from stdin into buffer */
	const size_t initialBufSize = 0x1000;
	size_t bufAlloc = initialBufSize;
	size_t bufUsage = 0;
	ssize_t bytesRead;
	char* buf = malloc(bufAlloc);

	while(1)
	{
		bytesRead = read(STDIN_FILENO, buf + bufUsage, bufAlloc - bufUsage);
		if(bytesRead < 0)
		{
			printf("read error...\n");
			exit(1);
		}
		if(bytesRead == 0)
			break;

		bufUsage += bytesRead;
		if(bufAlloc - bufUsage < 0x10)
		{
			size_t newAlloc = bufAlloc * 2;
			char* newBuf = realloc(buf, newAlloc);
			if(!newBuf)
				exit(1);
			buf = newBuf;
			bufAlloc = newAlloc;
		}
	}

	/* Give fuzzy input to driver */
	DRIVER_PTR_ID((const uint8_t*)buf, bufUsage);
	free(buf);
	return 0;
}
#endif
