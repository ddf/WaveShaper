#include "IPlugPlatform.h"
#include "FileLoader.h"

#ifdef OS_WIN
#include <windows.h>
#endif

// implements sf_virutal_io to allow us to use libsndfile to load wave files included as resources
struct ResourceFile
{
	const char * data;
	sf_count_t position;
	sf_count_t size;

	static sf_count_t getLength(void* user)
	{
		return static_cast<ResourceFile*>(user)->size;
	}

	static sf_count_t tell(void* user)
	{
		return static_cast<ResourceFile*>(user)->position;
	}

	static sf_count_t read(void* ptr, sf_count_t count, void* user)
	{
		ResourceFile* file = static_cast<ResourceFile*>(user);
		if (file->position + count >= file->size)
		{
			count = file->size - file->position;
		}

		memcpy(ptr, file->data + file->position, static_cast<size_t>(count));
		file->position += count;
		return count;
	}

	static sf_count_t seek(sf_count_t offset, int whence, void* user)
	{
		ResourceFile* file = static_cast<ResourceFile*>(user);
		sf_count_t seekPosition = 0;
		switch (whence)
		{
			case SEEK_SET: seekPosition = offset; break;
			case SEEK_CUR: seekPosition = file->position + offset; break;
			case SEEK_END: seekPosition = file->size - offset; break;
			default: seekPosition = 0; break;
		}

		if (seekPosition >= file->size)
		{
			seekPosition = file->size - 1;
		}
		else if (seekPosition < 0)
		{
			seekPosition = 0;
		}

		file->position = seekPosition;
		return seekPosition;
	}
};

FileLoader::FileLoader()
	: mBuffer(nullptr)
	, mBufferSize(0)
{

}

void FileLoader::Load(int resourceID, const char * resourceName, Minim::MultiChannelBuffer& outBuffer)
{
#ifdef OS_WIN
	HRSRC myResource = ::FindResource(NULL, MAKEINTRESOURCE(resourceID), "WAVE");
	unsigned int myResourceSize = ::SizeofResource(NULL, myResource);
	HGLOBAL myResourceData = ::LoadResource(NULL, myResource);
	void* pMyBinaryData = ::LockResource(myResourceData);

	ResourceFile resFile;
	resFile.data = static_cast<const char*>(pMyBinaryData);
	resFile.position = 0;
	resFile.size = myResourceSize;

	SF_VIRTUAL_IO io;
	io.get_filelen = &ResourceFile::getLength;
	io.read = &ResourceFile::read;
	io.seek = &ResourceFile::seek;
	io.tell = &ResourceFile::tell;

	SF_INFO fileInfo;
	fileInfo.format = 0;
	SNDFILE* file = sf_open_virtual(&io, SFM_READ, &fileInfo, &resFile);

	if (file != NULL)
	{
		ReadFile(fileInfo, file, outBuffer);
	}

	sf_close(file);

	::FreeResource(myResourceData);
#else
#endif
}

void FileLoader::Load(const char * fileName, Minim::MultiChannelBuffer& outBuffer)
{
	SF_INFO fileInfo;
	fileInfo.format = 0;
	SNDFILE* file = sf_open(fileName, SFM_READ, &fileInfo);

	if ( file != NULL )
	{
		ReadFile(fileInfo, file, outBuffer);
	}

	sf_close(file);
}

void FileLoader::ReadFile(SF_INFO& fileInfo, SNDFILE* file, Minim::MultiChannelBuffer& outBuffer)
{
	const sf_count_t fileSize = fileInfo.channels*fileInfo.frames;
	if (mBufferSize < fileSize)
	{
		if (mBuffer != nullptr)
		{
			delete[] mBuffer;
		}
		mBuffer = new float[fileSize];
	}
	// read in the whole thing
	sf_count_t framesRead = sf_readf_float(file, mBuffer, fileInfo.frames);

	outBuffer.setChannelCount(fileInfo.channels);
	outBuffer.setBufferSize(framesRead);
	// and now we should be able to de-interleave our read buffer into buffer
	for (int c = 0; c < fileInfo.channels; ++c)
	{
		float * channel = outBuffer.getChannel(c);
		for (int i = 0; i < framesRead; ++i)
		{
			const int offset = (i * fileInfo.channels) + c;
			const float sample = mBuffer[offset];
			channel[i] = sample;
		}
	}
}
