#include "lzmautil.h"

#include <filesystem>
#include <string>

size_t LzmaUtil::write_size_t = 0;
size_t LzmaUtil::read_size_t = 0;

size_t LzmaUtil::lzmaCompress(const uint8_t* input, uint64_t inputSize, mc::ByteStream* bytesteam) {
	CByteSeqOutStream outStream;
	outStream.funcTable.Write = MyByteWrite;
	outStream.bytesteam = bytesteam;

	CLzma2EncHandle encoder = Lzma2Enc_Create(&util_g_Alloc, &util_g_Alloc);
	// set up properties
	CLzma2EncProps props;
	Lzma2EncProps_Init(&props);
	props.lzmaProps.dictSize = inputSize;
	props.lzmaProps.level  = 9;
	props.numTotalThreads = 1;

	Lzma2Enc_SetProps(encoder, &props);

	//Lzma2Enc_SetDataSize(&props,inputSize);

	Byte header[1 + 8];
	int i;

	header[0] = Lzma2Enc_WriteProperties(encoder);
	for (i = 0; i < 8; i++)
		header[i+1] = (Byte)(inputSize >> (8 * i));
	
	bytesteam->write((char*)header,9);
	size_t outBufSize = 0;
	auto res = Lzma2Enc_Encode2(encoder,&outStream.funcTable,NULL,&outBufSize,NULL,input,inputSize,NULL);
	return res;
}

size_t LzmaUtil::lzmaDecompress(const uint8_t* input, uint64_t inputSize, mc::ByteStream* bytesteam) {
	CByteSeqOutStream outStream;
	outStream.funcTable.Write = MyByteWrite;
	outStream.bytesteam = bytesteam;

	mc::ByteStream inbytestream;
	inbytestream.write((char*)input,inputSize);
	CByteSeqInStream inStream;
	inStream.funcTable.Read = MyByteRead;
	inStream.bytesteam = &inbytestream;

	
	CLzma2DecMtHandle decoder = Lzma2DecMt_Create(&util_g_Alloc, &util_g_Alloc);

	char* header = new char[9];
	inbytestream.read(header,9);

	Byte prop = header[0];

	CLzma2DecMtProps props;
    Lzma2DecMtProps_Init(&props);
    props.inBufSize_ST = inbytestream.size();

	UInt64 outBufferSize = 0;
    for (int i = 0; i < 8; i++) {
      outBufferSize |= (header[1 + i] << (i * 8));
    }

	//Lzma2Enc_SetDataSize(&props,outBufferSize);
	UInt64 inProcessed = 0;
	int isMT = false;
    auto res = Lzma2DecMt_Decode(decoder, prop, &props, &outStream.funcTable, &outBufferSize, 1, &inStream.funcTable,
                                 &inProcessed, &isMT, nullptr);
	
	return res;
}

size_t LzmaUtil::EncodeFile(const char* inFile, const char* outFile,size_t* inputFileSize, bool append /* = false */) {
	CFileSeqInStream inStream;
	CFileOutStream outStream;
	int res;

	FileSeqInStream_CreateVTable(&inStream);
	File_Construct(&inStream.file);

	FileOutStream_CreateVTable(&outStream);
	File_Construct(&outStream.file);

	if (InFile_Open(&inStream.file, inFile) != 0)
	{
		return LZMA_STATUS_OPEN_FILE_FAIL;
	}

	if (MyOpenWirteFile(&outStream.file, outFile, append) != 0)
	{
		return LZMA_STATUS_OPEN_FILE_FAIL;
	}
	UInt64 fileSize;
	File_GetLength(&inStream.file, &fileSize);
	*inputFileSize = fileSize;
	res = Encode_lzma(&outStream.vt, &inStream.vt, fileSize);

	//if (res != SZ_OK)
	//{
	//	printf("File Encode error\r\n");

	//	if (res == SZ_ERROR_MEM)
	//		printf("SZ_ERROR_MEM\r\n");
	//	else if (res == SZ_ERROR_DATA)
	//		printf("SZ_ERROR_DATA\r\n");
	//	else if (res == SZ_ERROR_WRITE)
	//		printf("SZ_ERROR_WRITE\r\n");
	//	else if (res == SZ_ERROR_READ)
	//		printf("SZ_ERROR_READ\r\n");
	//}

	File_Close(&outStream.file);
	File_Close(&inStream.file);

	return res;
}

size_t LzmaUtil::EncodeFileToByte(const char* inFile, mc::ByteStream* bytesteam) {
	CFileSeqInStream inStream;
	CByteSeqOutStream outStream;
	int res;

	FileSeqInStream_CreateVTable(&inStream);
	File_Construct(&inStream.file);

	if (InFile_Open(&inStream.file, inFile) != 0)
	{
		return LZMA_STATUS_OPEN_FILE_FAIL;
	}

	outStream.funcTable.Write = MyByteWrite;
	outStream.bytesteam = bytesteam;

	UInt64 fileSize;
	File_GetLength(&inStream.file, &fileSize);
	res = Encode_lzma(&outStream.funcTable, &inStream.vt, fileSize);

	//if (res != SZ_OK)
	//{
	//	printf("File Encode error\r\n");

	//	if (res == SZ_ERROR_MEM)
	//		printf("SZ_ERROR_MEM\r\n");
	//	else if (res == SZ_ERROR_DATA)
	//		printf("SZ_ERROR_DATA\r\n");
	//	else if (res == SZ_ERROR_WRITE)
	//		printf("SZ_ERROR_WRITE\r\n");
	//	else if (res == SZ_ERROR_READ)
	//		printf("SZ_ERROR_READ\r\n");
	//}

	File_Close(&inStream.file);

	return res;
}

WRes LzmaUtil::MyOpenWirteFile(CSzFile* p, const char* name, bool append) {
	std::string filepath = name;
	if (std::filesystem::exists(filepath) && append) {
#ifdef USE_WINDOWS_FILE
		p->handle = CreateFileA(name,
			FILE_APPEND_DATA,
			FILE_SHARE_READ,
			NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		return (p->handle != INVALID_HANDLE_VALUE) ? 0 : GetLastError();
#else
		p->file = fopen(name, "ab+");
		return (p->file != 0) ? 0 : 2;
#endif
	}
	return OutFile_Open(p, name);
}

SRes LzmaUtil::Encode_lzma(ISeqOutStream* outStream, ISeqInStream* inStream, UInt64 fileSize) {
	CLzmaEncHandle enc;
	SRes res;
	CLzmaEncProps props;

	enc = LzmaEnc_Create(&util_g_Alloc);
	if (enc == 0)
		return SZ_ERROR_MEM;

	LzmaEncProps_Init(&props);
	//printf("LzmaEncProps_Init dictSize:%d level:%d numThreads:%d\r\n", props.dictSize, props.level, props.numThreads);

	/**
	 * 1:   Extremely fast
	 * 3:   fast
	 * 5:   standard
	 * 7:   maximum
	 * 9    ultimate compression
	 */
	props.level = 1;    // 1.
	props.dictSize = 1 << 16;
	props.numThreads = 1;

	res = LzmaEnc_SetProps(enc, &props);

	//printf("LzmaEnc_SetProps dictSize:%d level:%d numThreads:%d\r\n", props.dictSize, props.level, props.numThreads);

	if (res == SZ_OK)
	{
		Byte header[LZMA_PROPS_SIZE + 8];
		size_t headerSize = LZMA_PROPS_SIZE;
		int i;

		res = LzmaEnc_WriteProperties(enc, header, &headerSize);
		for (i = 0; i < 8; i++)
			header[headerSize++] = (Byte)(fileSize >> (8 * i));
		//printf(" ---> write header:%d fileSize:%d\r\n", headerSize, fileSize);
		if (outStream->Write(outStream, header, headerSize) != headerSize)
			res = SZ_ERROR_WRITE;
		else
		{
			if (res == SZ_OK)
			{
				//printf(" ---> start encode\r\n");
				res = LzmaEnc_Encode(enc, outStream, inStream, NULL, &util_g_Alloc, &util_g_Alloc);
			}
		}
	}
	LzmaEnc_Destroy(enc, &util_g_Alloc, &util_g_Alloc);
	return res;
}

size_t LzmaUtil::MyByteWrite(const ISeqOutStream* pp, const void* buf, size_t size) {
	mc::ByteStream* bs = ((CByteSeqOutStream*)pp)->bytesteam;
	void* data = const_cast<void*>(buf);
	bs->write((char*)data, size);
	return size;
}

SRes LzmaUtil::MyByteRead(const ISeqInStream *p, void *buf, size_t *size){
	if (*size == 0)   
		return SZ_OK;
	mc::ByteStream* bs = ((CByteSeqInStream*)p)->bytesteam;
	mc::BS_DSIZE bsReadSize = bs->read((char*)buf, *size);
	if(bsReadSize==0){
		return SZ_ERROR_READ;
	}
	return SZ_OK;
}

//static size_t DecodeFile(FILE* inFile, FILE* outFile)
//{

//	write_size_t = 0;
//	read_size_t = 0;

//	UInt64 unpackSize;
//	int thereIsSize; /* = 1, if there is uncompressed size in headers */
//	int i;
//	int res = 0;

//	CLzmaDec state;

//	/* header: 5 bytes of LZMA properties and 8 bytes of uncompressed size */
//	unsigned char header[LZMA_PROPS_SIZE + 8];

//	/* Read and parse header */

//	if (!MyReadFileAndCheck(inFile, header, sizeof(header)))
//		return -1;

//	unpackSize = 0;
//	thereIsSize = 0;
//	for (i = 0; i < 8; i++)
//	{
//		unsigned char b = header[LZMA_PROPS_SIZE + i];
//		if (b != 0xFF)
//			thereIsSize = 1;
//		unpackSize += (UInt64)b << (i * 8);
//	}

//	LzmaDec_Construct(&state);
//	res = LzmaDec_Allocate(&state, header, LZMA_PROPS_SIZE, &util_g_Alloc);
//	if (res != SZ_OK)
//		return res;
//	{
//		Byte inBuf[IN_BUF_SIZE];
//		Byte outBuf[OUT_BUF_SIZE];
//		size_t inPos = 0, inSize = 0, outPos = 0;
//		LzmaDec_Init(&state);
//		for (;;)
//		{
//			if (inPos == inSize)
//			{
//				inSize = MyReadFile(inFile, inBuf, IN_BUF_SIZE);
//				inPos = 0;
//			}
//			{
//				SizeT inProcessed = inSize - inPos;
//				SizeT outProcessed = OUT_BUF_SIZE - outPos;
//				ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
//				ELzmaStatus status;
//				if (thereIsSize && outProcessed > unpackSize)
//				{
//					outProcessed = (SizeT)unpackSize;
//					finishMode = LZMA_FINISH_END;
//				}

//				res = LzmaDec_DecodeToBuf(&state, outBuf + outPos, &outProcessed,
//					inBuf + inPos, &inProcessed, finishMode, &status);
//				inPos += (UInt32)inProcessed;
//				outPos += outProcessed;
//				unpackSize -= outProcessed;

//				if (outFile != 0)
//					MyWriteFile(outFile, outBuf, outPos);
//				outPos = 0;

//				if (res != SZ_OK || thereIsSize && unpackSize == 0)
//					break;

//				if (inProcessed == 0 && outProcessed == 0)
//				{
//					if (thereIsSize || status != LZMA_STATUS_FINISHED_WITH_MARK)
//						res = SZ_ERROR_DATA;
//					break;
//				}
//			}
//		}
//	}

//	LzmaDec_Free(&state, &util_g_Alloc);
//	if (res != SZ_OK) {
//		return -1001;
//	}
//	return write_size_t;
//}

	//---------------------------------------------------
	//static void hexdump(const uint8_t* buf, int size) {
	//	int lines = (size + 15) / 16;
	//	for (int i = 0; i < lines; i++) {
	//		printf("%08x | ", i * 16);

	//		int lineMin = i * 16;
	//		int lineMax = lineMin + 16;
	//		int lineCappedMax = (lineMax > size) ? size : lineMax;

	//		for (int j = lineMin; j < lineCappedMax; j++)
	//			printf("%02x ", buf[j]);
	//		for (int j = lineCappedMax; j < lineMax; j++)
	//			printf("   ");

	//		printf("| ");

	//		for (int j = lineMin; j < lineCappedMax; j++) {
	//			if (buf[j] >= 32 && buf[j] <= 127)
	//				printf("%c", buf[j]);
	//			else
	//				printf(".");
	//		}
	//		printf("\n");
	//	}
	//}


	//static void testIt(const uint8_t* input, int size) {
	//	printf("Test Input:\n");
	//	hexdump(input, size);

	//	uint64_t compressedSize;
	//	auto compressedBlob = lzmaCompress(input, size, &compressedSize);

	//	if (compressedBlob) {
	//		printf("Compressed:\n");
	//		hexdump(compressedBlob.get(), compressedSize);
	//	}
	//	else {
	//		printf("Nope, we screwed it\n");
	//		return;
	//	}

	//	// let's try decompressing it now
	//	uint64_t decompressedSize;
	//	auto decompressedBlob = lzmaDecompress(compressedBlob.get(), compressedSize, &decompressedSize, compressedSize);

	//	if (decompressedBlob) {
	//		printf("Decompressed:\n");
	//		hexdump(decompressedBlob.get(), decompressedSize);
	//	}
	//	else {
	//		printf("Nope, we screwed it (part 2)\n");
	//		return;
	//	}

	//	printf("----------\n");
	//}

	//static void testIt(const char* string) {
	//	testIt((const uint8_t*)string, strlen(string));
	//}


	//static size_t MyReadFile(FILE* file, void* data, size_t size)
	//{
	//	size_t readsize = fread(data, 1, size, file);

	//	read_size_t = read_size_t + readsize;

	//	return readsize;
	//}

	//static int MyReadFileAndCheck(FILE* file, void* data, size_t size)
	//{
	//	return (MyReadFile(file, data, size) == size);
	//}

	//static size_t MyWriteFile(FILE* file, const void* data, size_t size)
	//{
	//	if (size == 0)
	//		return 0;

	//	size_t writesize = fwrite(data, 1, size, file);

	//	write_size_t = write_size_t + writesize;

	//	return writesize;
	//}

	//static int MyWriteFileAndCheck(FILE* file, const void* data, size_t size)
	//{
	//	return (MyWriteFile(file, data, size) == size);
	//}

	//static long MyGetFileLength(FILE* file)
	//{
	//	long length;
	//	fseek(file, 0, SEEK_END);
	//	length = ftell(file);
	//	fseek(file, 0, SEEK_SET);
	//	return length;
	//}

	//static SRes MyRead(ISeqInStream* p, void* buf, size_t* size)
	//{
	//	if (*size == 0)
	//		return SZ_OK;
	//	*size = MyReadFile(((CFileSeqInStream*)p)->file, buf, *size);
	//	/*
	//	if (*size == 0)
	//	  return SZE_FAIL;
	//	*/
	//	return SZ_OK;
	//}

	//static size_t MyWrite(void* pp, const void* buf, size_t size)
	//{
	//	return MyWriteFile(((CFileSeqOutStream*)pp)->file, buf, size);
	//}

	//static size_t EncodeFileToByte(FILE* inFile, Bitstream* bitstream)
	//{
	//	write_size_t = 0;
	//	read_size_t = 0;

	//	CLzmaEncHandle enc;
	//	SRes res;
	//	CFileSeqInStream inStream;
	//	CByteSeqOutStream outStream;
	//	CLzmaEncProps props;

	//	enc = LzmaEnc_Create(&util_g_Alloc);
	//	if (enc == 0)
	//		return -1;

	//	inStream.funcTable.Read = MyRead;
	//	inStream.file = inFile;
	//	outStream.funcTable.Write = MyByteWrite;
	//	outStream.bitstream = bitstream;

	//	LzmaEncProps_Init(&props);
	//	res = LzmaEnc_SetProps(enc, &props);

	//	if (res == SZ_OK)
	//	{
	//		Byte header[LZMA_PROPS_SIZE + 8];
	//		size_t headerSize = LZMA_PROPS_SIZE;
	//		UInt64 fileSize;
	//		int i;

	//		res = LzmaEnc_WriteProperties(enc, header, &headerSize);
	//		fileSize = MyGetFileLength(inFile);
	//		for (i = 0; i < 8; i++){
	//			header[headerSize++] = (Byte)(fileSize >> (8 * i));
	//		}
	//		bitstream->WriteArray<Byte>(header, headerSize);

	//		if (res == SZ_OK)
	//			res = LzmaEnc_Encode(enc, &outStream.funcTable, &inStream.funcTable,
	//				NULL, &util_g_Alloc, &util_g_Alloc);
	//	}
	//	LzmaEnc_Destroy(enc, &util_g_Alloc, &util_g_Alloc);
	//	if (res != SZ_OK) {
	//		return -1001;
	//	}
	//	return write_size_t;
	//}