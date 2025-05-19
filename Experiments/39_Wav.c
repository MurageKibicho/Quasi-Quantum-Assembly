#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
typedef struct stereoaudiobuffer_struct StereoAudioBuffer;
typedef struct ChunkHeader ChunkHeader;
typedef struct FmtChunk FmtChunk;
/* Chunk Header Structure */
struct ChunkHeader
{
	char id[4];
	uint32_t size;
};


/* Format Chunk Structure */
struct FmtChunk
{
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
};


struct stereoaudiobuffer_struct
{
	float *left;
	float *right;
	uint32_t sampleCount;
};

StereoAudioBuffer ConvertToFloatSamplesStereo(void* rawData, uint32_t totalSampleFrames, uint16_t bitsPerSample)
{
	StereoAudioBuffer buffer = {NULL, NULL, 0};
	uint32_t sampleCount = totalSampleFrames * 2; // 2 channels

	buffer.left = (float*)malloc(totalSampleFrames * sizeof(float));
	buffer.right = (float*)malloc(totalSampleFrames * sizeof(float));
	buffer.sampleCount = totalSampleFrames;
	assert(buffer.left != NULL && buffer.right != NULL);

	switch(bitsPerSample)
	{
		case 16:
		{ 
			int16_t *samples = (int16_t*)rawData;
			for(uint32_t i = 0, frame = 0; frame < totalSampleFrames; frame++)
			{
				buffer.left[frame] = samples[i++] / 32768.0f;
				buffer.right[frame] = samples[i++] / 32768.0f;
			}
			break;
		}
		case 24:
		{
			uint8_t* bytes = (uint8_t*)rawData;
			for(uint32_t i = 0, frame = 0; frame < totalSampleFrames; frame++)
			{
				int32_t left = (bytes[i+2] << 16) | (bytes[i+1] << 8) | bytes[i];
				if (left & 0x800000) left |= 0xFF000000; // Sign extend
				i += 3;
				
				int32_t right = (bytes[i+2] << 16) | (bytes[i+1] << 8) | bytes[i];
				if (right & 0x800000) right |= 0xFF000000; // Sign extend
				i += 3;

				buffer.left[frame] = left / 8388608.0f;
				buffer.right[frame] = right / 8388608.0f;
			}
			break;
		}
		case 32:
		{
			int32_t* samples = (int32_t*)rawData;
			for(uint32_t i = 0, frame = 0; frame < totalSampleFrames; frame++)
			{
				buffer.left[frame] = samples[i++] / 2147483648.0f;
				buffer.right[frame] = samples[i++] / 2147483648.0f;
			}
			break;
		}
		default:
			fprintf(stderr, "Unsupported bit depth: %u\n", bitsPerSample);
			free(buffer.left);
			free(buffer.right);
			buffer.left = buffer.right = NULL;
	}
	return buffer;
}

void ModelStereo(StereoAudioBuffer buffer)
{
	//for(uint32_t x = 0; x < buffer.sampleCount; x++)
	for(uint32_t x = 0; x < 100; x++)
    	{
    		float y = buffer.left[x];
    		printf("%u %.3f\n", x, y);
    	}
	
}

void visualize_stereo_wav(StereoAudioBuffer buffer, const char* output_path)
{
	const int width = 800;
	const int height = 400;
	const int channels = 3;
	unsigned char* pixels = calloc(width * height * channels, sizeof(unsigned char));

    	// Draw stereo waveform (left=red, right=cyan)
    	for(int i = 0; i < buffer.sampleCount; i++)
    	{
		int x = (int)((float)i / buffer.sampleCount * width);

		// Left channel (red)
		int y_left = height/2 + (int)(buffer.left[i] * height/2 * 0.8f);
		if(y_left >= 0 && y_left < height)
		{
			unsigned char* p = pixels + (y_left * width + x) * channels;
			p[0] = 255; // R
			p[1] = 0;   // G
			p[2] = 0;   // B
		}

		// Right channel (cyan)
		/*int y_right = height/2 + (int)(buffer.right[i] * height/2 * 0.8f);
		if(y_right >= 0 && y_right < height)
		{
			unsigned char* p = pixels + (y_right * width + x) * channels;
			p[0] = 0;   // R
			p[1] = 255; // G
			p[2] = 255; // B
		}*/
	}

	stbi_write_png(output_path, width, height, channels, pixels, width * channels);
	free(pixels);
}
int main()
{
	const char* fileName = "../Datasets/Audio/sample.wav";

	int fileDesc = open(fileName, O_RDONLY);
	assert(fileDesc != -1);

	struct stat fileStat;
	assert(fstat(fileDesc, &fileStat) != -1);

	void *mappedData = mmap(NULL, fileStat.st_size, PROT_READ, MAP_PRIVATE, fileDesc, 0);
	assert(mappedData != MAP_FAILED);

	/* Check RIFF header */
	ChunkHeader* riffHeader = (ChunkHeader*)mappedData;
	assert(strncmp(riffHeader->id, "RIFF", 4) == 0);
	assert(strncmp((char*)mappedData + 8, "WAVE", 4) == 0);

	FmtChunk* fmtChunk = NULL;
	void* dataChunk = NULL;
	uint32_t dataSize = 0;

	/* Process chunks */
	char* currentPos = (char*)mappedData + 12;
	while(currentPos < (char*)mappedData + fileStat.st_size)
	{
		ChunkHeader* chunk = (ChunkHeader*)currentPos;
		currentPos += sizeof(ChunkHeader);
		if(strncmp(chunk->id, "fmt ", 4) == 0)
		{
		    fmtChunk = (FmtChunk*)currentPos;
		}
		else if (strncmp(chunk->id, "data", 4) == 0)
		{
		    dataChunk = currentPos;
		    dataSize = chunk->size;
		}

		currentPos += chunk->size;
		if(chunk->size % 2) currentPos++;
	}

    	assert(fmtChunk != NULL);
    	assert(dataChunk != NULL);

	/* Print file info */
	printf("WAV File Information for %s:\n", fileName);
	printf("  Format: %s\n", fmtChunk->audioFormat == 1 ? "PCM" : "Compressed");
	printf("  Channels: %u\n", fmtChunk->numChannels);
	printf("  Sample Rate: %u Hz\n", fmtChunk->sampleRate);
	printf("  Bits per Sample: %u\n", fmtChunk->bitsPerSample);
	printf("  Data Size: %u bytes\n", dataSize);

	if(fmtChunk->audioFormat == 1)
	{
		double duration = (double)dataSize / fmtChunk->byteRate;
		printf("  Duration: %.2f seconds\n", duration);
	}

	assert(fmtChunk->numChannels == 2);
	uint32_t totalFrames = dataSize / (fmtChunk->numChannels * (fmtChunk->bitsPerSample / 8));
	StereoAudioBuffer stereoData = ConvertToFloatSamplesStereo(dataChunk, totalFrames,fmtChunk->bitsPerSample);

	if(stereoData.left && stereoData.right)
	{
		ModelStereo(stereoData);
		//visualize_stereo_wav(stereoData, "stereo_waveform.png");
	}
	free(stereoData.left);
	free(stereoData.right);
	assert(munmap(mappedData, fileStat.st_size) == 0);
	assert(close(fileDesc) == 0);
	return 0;
}
