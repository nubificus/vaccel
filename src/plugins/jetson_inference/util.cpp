#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <jetson-utils/cudaMappedMemory.h>
#include <jetson-utils/filesystem.h>

bool loadImageBufRGBA(void *buffer, int buf_len, float4 **cpu, float4 **gpu,
		int *width, int *height, const float4& mean)
{
	// validate parameters
	if (!buffer || !cpu || !gpu || !width || !height) {
		fprintf(stderr, "loadImageRGBA() - invalid parameter(s)\n");
		return false;
	}

	// attempt to load the data from disk
	int imgWidth = 0;
	int imgHeight = 0;
	int imgChannels = 0;

	unsigned char* img = stbi_load_from_memory((unsigned char *)buffer,
			buf_len, &imgWidth, &imgHeight, &imgChannels, 0);
	if (!img) {
		fprintf(stderr, "Failed to load img from memory\n");
		return false;
	}

	if (*width > 0 && *height > 0) {
		// TODO: Resize
	}

	// allocate CUDA buffer for the image
	const size_t imgSize = imgWidth * imgHeight * sizeof(float) * 4;

	if (!cudaAllocMapped((void **)cpu, (void **)gpu, imgSize)) {
		fprintf(stderr, "Failed to allocate %zu bytes for image\n", imgSize);
		return false;
	}


	// convert uint8 image to float4
	float4 *cpuPtr = *cpu;
	for (int y = 0; y < imgHeight; y++) {
		const size_t yOffset = y * imgWidth * imgChannels * sizeof(unsigned char);

		for (int x = 0; x < imgWidth; x++) {
			#define GET_PIXEL(channel) float(img[offset + channel])
			#define SET_PIXEL_FLOAT4(r,g,b,a) cpuPtr[y*imgWidth+x] = make_float4(r,g,b,a)

			const size_t offset = yOffset + x * imgChannels * sizeof(unsigned char);

			switch (imgChannels) {
			case 1:
			{
				const float grey = GET_PIXEL(0);
				SET_PIXEL_FLOAT4(grey - mean.x, grey - mean.y, grey - mean.z, 255.0f - mean.w);
				break;
			}
			case 2:
			{
				const float grey = GET_PIXEL(0);
				SET_PIXEL_FLOAT4(grey - mean.x, grey - mean.y, grey - mean.z, GET_PIXEL(1) - mean.w);
				break;
			}
			case 3:
			{
				SET_PIXEL_FLOAT4(GET_PIXEL(0) - mean.x, GET_PIXEL(1) - mean.y, GET_PIXEL(2) - mean.z, 255.0f - mean.w);
				break;
			}
			case 4:
			{
				SET_PIXEL_FLOAT4(GET_PIXEL(0) - mean.x, GET_PIXEL(1) - mean.y, GET_PIXEL(2) - mean.z, GET_PIXEL(3) - mean.w);
				break;
			}
			}
		}
	}

	*width  = imgWidth;
	*height = imgHeight;

	free(img);
	return true;
}

// limit_pixel
static inline unsigned char limit_pixel( float pixel, float max_pixel )
{
	if( pixel < 0 )
		pixel = 0;

	if( pixel > max_pixel )
		pixel = max_pixel;

	return (unsigned char)pixel;
}

bool saveImageFileRGBA(const char *filename, float4 *cpu, int width, int height, float max_pixel, int quality)
{
	// validate parameters
	if (!filename || !cpu || width <= 0 || height <= 0) {
		fprintf(stderr, "saveImageRGBA() - invalid parameter\n");
		return false;
	}

	if (quality < 1)
		quality = 1;

	if(quality > 100)
		quality = 100;

	// allocate memory for the uint8 image
	const size_t stride = width * sizeof(unsigned char) * 4;
	const size_t size   = stride * height;
	unsigned char *img  = (unsigned char *)malloc(size);

	if(!img) {
		fprintf(stderr, "failed to allocate %zu bytes to save %ix%i image '%s'\n", size, width, height, filename);
		return false;
	}

	// convert image from float to uint8
	const float scale = 255.0f / max_pixel;

	for(int y = 0; y < height; y++) {
		const size_t yOffset = y * stride;

		for(int x = 0; x < width; x++) {
			const size_t offset = yOffset + x * sizeof(unsigned char) * 4;
			const float4 pixel  = cpu[y * width + x];

			img[offset + 0] = limit_pixel(pixel.x * scale, max_pixel);
			img[offset + 1] = limit_pixel(pixel.y * scale, max_pixel);
			img[offset + 2] = limit_pixel(pixel.z * scale, max_pixel);
			img[offset + 3] = limit_pixel(pixel.w * scale, max_pixel);
		}
	}

	// determine the file extension
	const std::string ext = fileExtension(filename);
	const char *extension = ext.c_str();

	if(ext.size() == 0) {
		fprintf(stderr, "invalid filename or extension, '%s'\n", filename);
		free(img);
		return false;
	}

	// save the image
	int save_result = 0;

	if (strcasecmp(extension, "jpg") == 0 || strcasecmp(extension, "jpeg") == 0) {
		save_result = stbi_write_jpg(filename, width, height, 4, img, quality);
	}
	else if (strcasecmp(extension, "png") == 0) {
		// convert quality from 1-100 to 0-9 (where 0 is high quality)
		quality = (100 - quality) / 10;

		if (quality < 0)
			quality = 0;

		if (quality > 9)
			quality = 9;

		stbi_write_png_compression_level = quality;

		// write the PNG file
		save_result = stbi_write_png(filename, width, height, 4, img, stride);
	}
	else if (strcasecmp(extension, "tga") == 0) {
		save_result = stbi_write_tga(filename, width, height, 4, img);
	}
	else if (strcasecmp(extension, "bmp") == 0) {
		save_result = stbi_write_bmp(filename, width, height, 4, img);
	}
	else if (strcasecmp(extension, "hdr") == 0) {
		save_result = stbi_write_hdr(filename, width, height, 4, (float*)cpu);
	}
	else {
		fprintf(stderr, "invalid extension format '.%s' saving image '%s'\n", extension, filename);
		fprintf(stderr, "valid extensions are:  JPG/JPEG, PNG, TGA, BMP, and HDR.\n");

		free(img);
		return false;
	}

	// check the return code
	if (!save_result) {
		fprintf(stderr, "failed to save %ix%i image to '%s'\n", width, height, filename);
		free(img);
		return false;
	}

	free(img);
	return true;
}
