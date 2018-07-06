
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <assert.h>

#include "../src/Config.h"


size_t macke_fuzzer_array_byte_size(const uint8_t* src, size_t max)
{
	bool escaped = false;
	size_t len = 0;
	for(size_t i = 0; i < max; ++i, ++len)
	{
		if(escaped)
		{
			if(src[i] == DELIMITER_CHAR || src[i] == ESCAPE_CHAR)
				--len;
			escaped = false;
		}
		else
		{
			if(src[i] == DELIMITER_CHAR)
				return len;
			else if(src[i] == ESCAPE_CHAR)
				escaped = true;
		}
	}
	return len;
}


const uint8_t* macke_fuzzer_array_extract(const uint8_t* src, size_t srcLen, uint8_t* dst, size_t dstLen)
{
	size_t copied = 0;
	const uint8_t* lastSrc = src + srcLen - 1;
	while(copied < dstLen)
	{
		if(*src == ESCAPE_CHAR)
		{
			if(src == lastSrc)
			{
				*dst++ = *src++;
				++copied;
			}
			else if(src[1] == ESCAPE_CHAR || src[1] == DELIMITER_CHAR)
			{
				*dst++ = src[1];
				src += 2;
				++copied;
			}
			else
			{
				if(copied == dstLen - 1)
				{
					*dst++ = *src++;
					++copied;
				}
				else
				{
					*dst++ = *src++;
					*dst++ = *src++;
					copied += 2;
				}
			}
		}
		else if(*src == DELIMITER_CHAR)
		{
			assert(copied == dstLen);
			return src + 1;
		}
		else
		{
			*dst++ = *src++;
			++copied;
		}
	}

	assert(copied == dstLen);
	bool escaped = false;

	/* Search for delimiter char */
	while(src <= lastSrc)
	{
		if(escaped)
			escaped = false;
		else
			if(*src == DELIMITER_CHAR)
				return src + 1;
			else if(*src == ESCAPE_CHAR)
				escaped = true;
		++src;
	}
	return src;
}
