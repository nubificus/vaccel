#include <jetson-inference/imageNet.h>
#include <jetson-inference/detectNet.h>
#include <jetson-inference/segNet.h>
#include <jetson-utils/loadImage.h>
#include <jetson-utils/cudaFont.h>
#include <jetson-utils/cudaMappedMemory.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vaccel.h"
#include "util.h"

using namespace std;

#define IMAGENET_NETWORKS_DEFAULT_PATH "/usr/local/share/imagenet-models/networks"
#define IMAGENET_NETWORKS_ENVVAR "VACCEL_IMAGENET_NETWORKS"

/* Check if a path exists and is a directory */
static bool directory_exists(const char *path)
{
	struct stat s;
	return (!stat(path, &s) && (s.st_mode & S_IFDIR));
}

static const char *find_imagenet_models_path(void)
{
	/* Check first the environment variable */
	char *networks = getenv(IMAGENET_NETWORKS_ENVVAR);
	if (networks && directory_exists(networks))
		return networks;

	if (directory_exists(IMAGENET_NETWORKS_DEFAULT_PATH))
		return IMAGENET_NETWORKS_DEFAULT_PATH;

	return NULL;
}

int jetson_image_classification(struct vaccel_session *sess, const void *img,
		char *out_text, char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	/* Check for networks directory */
	const char *networks_dir = find_imagenet_models_path();
	int ret = VACCEL_OK;
	if (!networks_dir) {
		ret = VACCEL_ENOENT;
		goto out;
	}

	/* 512 bytes should be enough, but this is dangerous for failing
	 * spuriously */
	char prototxt_path[512], model_path[512], class_path[512];
	snprintf(prototxt_path, sizeof(prototxt_path), "%s/googlenet.prototxt",
			networks_dir);
	snprintf(model_path, sizeof(model_path), "%s/bvlc_googlenet.caffemodel",
			networks_dir);
	snprintf(class_path, sizeof(class_path), "%s/ilsvrc12_synset_words.txt",
			networks_dir);

	imageNet *net =
		imageNet::Create(prototxt_path, model_path, NULL, class_path,
			IMAGENET_DEFAULT_INPUT, IMAGENET_DEFAULT_OUTPUT,
			DEFAULT_MAX_BATCH_SIZE, TYPE_FASTEST, DEVICE_GPU,
			true);

	/*
	 * load image from disk
	 */
	float *imgCPU = NULL;
	float *imgCUDA = NULL;
	int imgWidth  = 0;
	int imgHeight = 0;

	if (!loadImageBufRGBA(img, len_img, (float4**)&imgCPU, (float4**)&imgCUDA, &imgWidth, &imgHeight, make_float4(0,0,0,0))) {
		fprintf(stderr, "Failed to load image\n");
		ret = VACCEL_ENOENT;
		goto close:
	}

	/*
	 * classify image
	 */
	float confidence = 0.0f;
	const int img_class = net->Classify(imgCUDA, imgWidth, imgHeight, &confidence);

	// overlay the classification on the image
	if (img_class >= 0) {
		printf("imagenet: %2.5f%% class #%i (%s)\n", confidence * 100.0f, img_class, net->GetClassDesc(img_class));

		const char *outputFilename = "processedImg.jpg";
		snprintf(out_imgname, len_out_imgname, "%s", outputFilename);

		if (outputFilename != NULL) {
			// use font to draw the class description
			cudaFont* font = cudaFont::Create(adaptFontSize(imgWidth));

			if (font != NULL) {
				char str[512];
				sprintf(str, "%2.3f%% %s", confidence * 100.0f, net->GetClassDesc(img_class));
				snprintf(out_text, len_out_text, "%s", str);

				font->OverlayText((float4 *)imgCUDA, imgWidth, imgHeight, (const char *)str, 10, 10,
							   make_float4(255, 255, 255, 255), make_float4(0, 0, 0, 100));
			}

			// wait for GPU to complete work
			CUDA(cudaDeviceSynchronize());

			// print out performance info
			//net->PrintProfilerTimes();

			// save the output image to disk
			printf("imagenet: attempting to save output image\n");

			if (!saveImageFileRGBA(outputFilename, (float4 *)imgCPU, imgWidth, imgHeight, 255.0f, 100))
				fprintf(stderr, "imagenet: failed to save output image\n");
			else
				printf("imagenet: completed saving\n");
		}
	}
	else {
		fprintf(stderr, "imagenet: failed to classify image (result=%i)\n", img_class);
	}

	/*
	 * destroy resources
	 */
	printf("imagenet: shutting down...\n");

close:
	CUDA(cudaFreeHost(imgCPU));
	SAFE_DELETE(net);
	CUDA(cudaDeviceReset());

out:
	return ret;
}

int jetson_image_detect(struct vaccel_session *sess, void *img,
		char *out_text, char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	detectNet *net = NULL;
	const char *modelName = NULL; //network;
	float threshold = 0.0f;
	int maxBatchSize = 0;

	if (!modelName)
		modelName = "ssd-mobilenet-v2";

	if (threshold == 0.0f)
		threshold = DETECTNET_DEFAULT_THRESHOLD;

	if (maxBatchSize < 1)
		maxBatchSize = DEFAULT_MAX_BATCH_SIZE;

	// parse the network type
	const detectNet::NetworkType type = detectNet::NetworkTypeFromStr(modelName);
/*
	if(type == detectNet::CUSTOM) {
		const char* prototxt =  cmdLine.GetString("prototxt");
		const char* input = cmdLine.GetString("input_blob");
		const char* output = cmdLine.GetString("output_blob");
		const char* out_cvg = cmdLine.GetString("output_cvg");
		const char* out_bbox = cmdLine.GetString("output_bbox");
		const char* class_labels = cmdLine.GetString("class_labels");

		if (!input)
			input = DETECTNET_DEFAULT_INPUT;
		if (!out_blob) {
			if (!out_cvg) out_cvg  = DETECTNET_DEFAULT_COVERAGE;
			if (!out_bbox) out_bbox = DETECTNET_DEFAULT_BBOX;
		}

		float meanPixel = cmdLine.GetFloat("mean_pixel");

		if (maxBatchSize < 1)
			maxBatchSize = DEFAULT_MAX_BATCH_SIZE;

		net = detectNet::Create(prototxt, modelName, meanPixel, class_labels, threshold, input,
		                                        out_blob ? NULL : out_cvg, out_blob ? out_blob : out_bbox, maxBatchSize);
	} else {
*/		// create from pretrained model
		net = detectNet::Create(type, threshold, maxBatchSize);
/*	}
*/

	if (!net) {
		fprintf(stderr, "Failed to initialize detectNet\n");
		return VACCEL_ENOENT;
	}
/*
	// enable layer profiling if desired
	if( cmdLine.GetFlag("profile") )
		net->EnableLayerProfiler();
*/

	// set overlay alpha value
	net->SetOverlayAlpha(DETECTNET_DEFAULT_ALPHA);

	const uint32_t overlayFlags = detectNet::OverlayFlagsFromStr("box,labels,conf");

	/*
	 * load image from disk
	 */
	float *imgCPU = NULL;
	float *imgCUDA = NULL;
	int imgWidth  = 0;
	int imgHeight = 0;

	if (!loadImageBufRGBA(img, len_img, (float4**)&imgCPU, (float4**)&imgCUDA, &imgWidth, &imgHeight, make_float4(0,0,0,0))) {
		fprintf(stderr, "Failed to load image\n");
		return VACCEL_ENOENT;
	}

	/*
	 * detect objects in image
	 */
	detectNet::Detection* detections = NULL;

	const int numDetections = net->Detect(imgCUDA, imgWidth, imgHeight, &detections, overlayFlags);

	// print out the detection results
	printf("detectnet: %i objects detected\n", numDetections);

	//for (int n=0; n < numDetections; n++) {
	//	printf("detected obj %u  class #%u (%s)  confidence=%f\n", detections[n].Instance, detections[n].ClassID, net->GetClassDesc(detections[n].ClassID), detections[n].Confidence);
	//	detections[n].Top, detections[n].Right, detections[n].Bottom, detections[n].Width(), detections[n].Height());
	//}

	// print out timing info
	// net->PrintProfilerTimes();

	const char *outputFilename = "processedImg.jpg";
	snprintf(out_imgname, len_out_imgname, "%s", outputFilename);

	if (outputFilename != NULL) {
		printf("detectnet:  writing %ix%i image to '%s'\n", imgWidth, imgHeight, outputFilename);

		if (!saveImageFileRGBA(outputFilename, (float4 *)imgCPU, imgWidth, imgHeight, 255.0f, 100))
			fprintf(stderr, "detectnet: failed to save output image\n");
		else
			printf("detectnet: completed saving\n");
	}

	/*
	 * destroy resources
	 */
	printf("detectnet: shutting down...\n");

	CUDA(cudaFreeHost(imgCPU));
	SAFE_DELETE(net);
	CUDA(cudaDeviceReset());

	return VACCEL_OK;
}

int jetson_image_segment(struct vaccel_session *sess, void *img,
		char *out_text, char *out_imgname,
		size_t len_img, size_t len_out_text, size_t len_out_imgname)
{
	segNet *net = NULL;
	const char *modelName = NULL; //network;

	if (!modelName)
		modelName = "fcn-resnet18-voc-320x320";

	// parse the network type
	const segNet::NetworkType type = segNet::NetworkTypeFromStr(modelName);
/*
	if(type == imageNet::CUSTOM) {
		const char* prototxt =  cmdLine.GetString("prototxt");
		const char* labels = cmdLine.GetString("labels");
		const char* colors = cmdLine.GetString("colors");
		const char* input = cmdLine.GetString("input_blob");
		const char* output = cmdLine.GetString("output_blob");

		if (!input)
			input = SEGNET_DEFAULT_INPUT;
		if (!output)
			output = SEGNET_DEFAULT_OUTPUT;

		int maxBatchSize = cmdLine.GetInt("batch_size");

		if( maxBatchSize < 1 )
			maxBatchSize = DEFAULT_MAX_BATCH_SIZE;

		net = segNet::Create(prototxt, modelName, NULL, labels, input, output, maxBatchSize);
	} else {
*/		// create from pretrained model
		net = segNet::Create(type);
/*	}
*/

	if(!net) {
		fprintf(stderr, "Failed to initialize segNet\n");
		return VACCEL_ENOENT;
	}
/*
	// save the legend if desired
	const char* legend = cmdLine.GetString("legend");
	if( legend != NULL )
		net->saveClassLegend(legend);

	// enable layer profiling if desired
	if( cmdLine.GetFlag("profile") )
		net->EnableLayerProfiler();
*/

	// set overlay alpha value
	net->SetOverlayAlpha(SEGNET_DEFAULT_ALPHA);

	// get the desired alpha blend filtering mode
	const segNet::FilterMode filterMode = segNet::FilterModeFromStr("linear");

	// get the object class to ignore (if any)
	const char* ignoreClass = "void";

	// get the visualization mode (mask or overlay)
	const char* visualization = "overlay";

	/*
	 * load image from disk
	 */
	float *imgCPU = NULL;
	float *imgCUDA = NULL;
	int imgWidth  = 0;
	int imgHeight = 0;

	if (!loadImageBufRGBA(img, len_img, (float4**)&imgCPU, (float4**)&imgCUDA, &imgWidth, &imgHeight, make_float4(0,0,0,0))) {
		fprintf(stderr, "Failed to load image\n");
		return VACCEL_ENOENT;
	}

	/*
	 * allocate output image
	 */
	float* outCPU  = NULL;
	float* outCUDA = NULL;

	if (!cudaAllocMapped((void**)&outCPU, (void**)&outCUDA, imgWidth * imgHeight * sizeof(float) * 4)) {
		fprintf(stderr, "segnet: failed to allocate CUDA memory for output image (%ix%i)\n", imgWidth, imgHeight);
		return VACCEL_ENOENT;
	}

	/*
	 * perform the segmentation
	 */
	if (!net->Process(imgCUDA, imgWidth, imgHeight, ignoreClass)) {
		fprintf(stderr, "segnet: failed to process segmentation\n");
		return VACCEL_ENOENT;
	}

	// generate image overlay
	if (strcasecmp(visualization, "mask") == 0) {
		if (!net->Mask(outCUDA, imgWidth, imgHeight, filterMode)) {
			fprintf(stderr, "segnet: failed to generate overlay.\n");
			return VACCEL_ENOENT;
		}
	} else {
		if (!net->Overlay(outCUDA, imgWidth, imgHeight, filterMode)) {
			fprintf(stderr, "segnet: failed to generate overlay.\n");
			return VACCEL_ENOENT;
		}
	}

	// wait for GPU to complete work
	CUDA(cudaDeviceSynchronize());

	// print out timing info
	// net->PrintProfilerTimes();

	const char *outputFilename = "processedImg.jpg";
	snprintf(out_imgname, len_out_imgname, "%s", outputFilename);

	if (outputFilename != NULL) {
		printf("segnet: writing %ix%i image to '%s'\n", imgWidth, imgHeight, outputFilename);

		if (!saveImageFileRGBA(outputFilename, (float4 *)imgCPU, imgWidth, imgHeight, 255.0f, 100))
			fprintf(stderr, "segnet: failed to save output image\n");
		else
			printf("segnet: completed saving\n");
	}

	/*
	 * destroy resources
	 */
	printf("segnet: shutting down...\n");

	CUDA(cudaFreeHost(imgCPU));
	CUDA(cudaFreeHost(outCPU));
	SAFE_DELETE(net);
	CUDA(cudaDeviceReset());

	return VACCEL_OK;
}
