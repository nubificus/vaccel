#ifndef _UTIL_H_
#define _UTIL_H_

#include <stddef.h>
#include <string>
#include <cuda.h>

// ******** Image Utility Functions ******************************************
struct ImageData
{
	size_t columns;
	size_t rows;
	size_t bytes;
	size_t padded_bytes;
	unsigned char* data;
};

ImageData ReadImageFile (char* name);
void WriteImageFile (char* name, ImageData img);

bool loadImageBufRGBA(void *buffer, int buf_len, float4 **cpu, float4 **gpu,
		int *width, int *height, const float4& mean);
bool saveImageFileRGBA(const char *filename, float4 *cpu, int width, int height,
		float max_pixel, int quality);

#endif
