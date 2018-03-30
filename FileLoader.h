#pragma once

#include "MultiChannelBuffer.h"

// helper class to load audio files from resources or from disk
class FileLoader
{
public:
	FileLoader();
	void Load(int resourceID, const char * resourceName, Minim::MultiChannelBuffer& outBuffer);	

private:
	float * mBuffer;
	size_t  mBufferSize;
};