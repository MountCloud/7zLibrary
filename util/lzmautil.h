#pragma once

#ifndef _LZMA_UTIL_H_
#define _LZMA_UTIL_H_

#include "7z/7zAlloc.h"
#include "7z/7zFile.h"
#include "7z/LzmaDec.h"
#include "7z/LzmaEnc.h"
#include "7z/Lzma2DecMt.h"
#include "7z/Lzma2Enc.h"

#include "io/bytestream.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <memory>
#include <string>
#include <vector>

// void* MyAlloc(size_t size);

// void MyFree(void* address);

// static void* SzAlloc(ISzAllocPtr p,LZMA_DATASIZE size);
// static void SzFree(ISzAllocPtr p, void* address);

#define LZMA_STATUS_OPEN_FILE_FAIL -1
#define LZMA_STATUS_FILE_NOT_EXIST -2

#define LZMA_LEVEL 9
#define LZMA_THREAD_NUM 4

typedef unsigned long long LZMA_DATASIZE;


const ISzAlloc util_g_Alloc = { SzAlloc, SzFree };

typedef struct _CByteSeqOutStream
{
	ISeqOutStream vt;
	mc::ByteStream* bytesteam;
} CByteSeqOutStream;

typedef struct _CByteSeqInStream
{
	ISeqInStream vt;
	mc::ByteStream* bytesteam;
} CByteSeqInStream;

class LzmaUtil {
public:

	static SRes lzmaCompress(const uint8_t* input, LZMA_DATASIZE inputSize, mc::ByteStream* bytesteam,bool writeheader = true);

	static SRes lzmaDecompress(const uint8_t* input, LZMA_DATASIZE inputSize, mc::ByteStream* bytesteam,LZMA_DATASIZE checkOriginalSize = 0,bool readheader = true);

	static SRes EncodeFile(std::string inFile, std::string outFile, LZMA_DATASIZE* inputFileSize,bool writeheader = true, bool append = false);

	static SRes EncodeFileToByte(std::string inFile , LZMA_DATASIZE* inputFileSize, mc::ByteStream* bytesteam,bool writeheader = true);

private:
	static WRes MyOpenWirteFile(CSzFile* p, const char* name, bool append);

	static size_t MyByteWrite(const ISeqOutStream* pp, const void* buf,size_t size);
	static SRes MyByteRead(const ISeqInStream *p, void *buf,size_t *size);


};
#endif