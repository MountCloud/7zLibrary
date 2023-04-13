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

// void* MyAlloc(size_t size);

// void MyFree(void* address);

// static void* SzAlloc(ISzAllocPtr p, size_t size);
// static void SzFree(ISzAllocPtr p, void* address);

#define LZMA_STATUS_OPEN_FILE_FAIL -1


const ISzAlloc util_g_Alloc = { SzAlloc, SzFree };

typedef struct _CByteSeqOutStream
{
	ISeqOutStream funcTable;
	mc::ByteStream* bytesteam;
} CByteSeqOutStream;

typedef struct _CByteSeqInStream
{
	ISeqInStream funcTable;
	mc::ByteStream* bytesteam;
} CByteSeqInStream;


class LzmaUtil {
public:

	static size_t lzmaCompress(const uint8_t* input, uint64_t inputSize, mc::ByteStream* bytesteam);

	static size_t lzmaDecompress(const uint8_t* input, uint64_t inputSize, mc::ByteStream* bytesteam);

	static size_t EncodeFile(const char* inFile, const char* outFile,size_t* inputFileSize, bool append = false);

	static size_t EncodeFileToByte(const char* inFile, mc::ByteStream* bytesteam);

private:
	static size_t write_size_t;
	static size_t read_size_t;

private:
	static WRes MyOpenWirteFile(CSzFile* p, const char* name, bool append);
	/**
	 * 压缩函数封装
	 */
	static SRes Encode_lzma(ISeqOutStream* outStream, ISeqInStream* inStream, UInt64 fileSize);

	static size_t MyByteWrite(const ISeqOutStream* pp, const void* buf, size_t size);
	static SRes MyByteRead(const ISeqInStream *p, void *buf, size_t *size);


};
#endif