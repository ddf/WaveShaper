#pragma once

#include "MultiChannelBuffer.h"
#include "sndfile.h"

// helper class to load audio files from resources or from disk
class FileLoader
{
public:
	FileLoader();
	void Load(int resourceID, const char * resourceName, Minim::MultiChannelBuffer& outBuffer);	
	void Load(const char * fileName, Minim::MultiChannelBuffer& outBuffer);

private:

	void ReadFile(SF_INFO& info, SNDFILE* file, Minim::MultiChannelBuffer& outBuffer);

	float * mBuffer;
	size_t  mBufferSize;
};