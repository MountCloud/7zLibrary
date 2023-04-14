#include "lzmautil.h"

#include <filesystem>
#include <string>


SRes LzmaUtil::lzmaCompress(const uint8_t* input, LZMA_DATASIZE inputSize, mc::ByteStream* bytesteam,bool writeheader) {
	CByteSeqOutStream outStream;
	outStream.vt.Write = MyByteWrite;
	outStream.bytesteam = bytesteam;

	CLzma2EncHandle encoder = Lzma2Enc_Create(&util_g_Alloc, &util_g_Alloc);
	// set up properties
	CLzma2EncProps props;
	Lzma2EncProps_Init(&props);
	props.lzmaProps.dictSize = inputSize;
	props.lzmaProps.level  = LZMA_LEVEL;
	props.numTotalThreads = LZMA_THREAD_NUM;

	Lzma2Enc_SetProps(encoder, &props);

	//Lzma2Enc_SetDataSize(&props,inputSize);
	if(writeheader){
		Byte header[1 + 8];
		int i;

		header[0] = Lzma2Enc_WriteProperties(encoder);
		for (i = 0; i < 8; i++)
			header[i+1] = (Byte)(inputSize >> (8 * i));
		
		bytesteam->write((char*)header,9);
	}
	LZMA_DATASIZE outBufSize = 0;
	auto res = Lzma2Enc_Encode2(encoder,&outStream.vt,NULL,&outBufSize,NULL,input,inputSize,NULL);
	return res;
}

SRes LzmaUtil::lzmaDecompress(const uint8_t* input, LZMA_DATASIZE inputSize, mc::ByteStream* bytesteam,LZMA_DATASIZE checkOriginalSize,bool readheader) {
	CByteSeqOutStream outStream;
	outStream.vt.Write = MyByteWrite;
	outStream.bytesteam = bytesteam;

	mc::ByteStream inbytestream;
	inbytestream.write((char*)input,inputSize);
	CByteSeqInStream inStream;
	inStream.vt.Read = MyByteRead;
	inStream.bytesteam = &inbytestream;

	
	CLzma2DecMtHandle decoder = Lzma2DecMt_Create(&util_g_Alloc, &util_g_Alloc);

	Byte prop = 0;
	UInt64 outBufferSize = 0;
	if(readheader){
		char* header = new char[9];
		inbytestream.read(header,9);

		Byte prop = header[0];
		
		for (int i = 0; i < 8; i++) {
			outBufferSize |= (header[1 + i] << (i * 8));
		}
		if(checkOriginalSize>0&&checkOriginalSize != outBufferSize){
			return SZ_ERROR_DATA;
		}
	}
	
	CLzma2DecMtProps props;
	Lzma2DecMtProps_Init(&props);
	props.inBufSize_ST = inbytestream.size();
	props.numThreads = LZMA_THREAD_NUM;

	//Lzma2Enc_SetDataSize(&props,outBufferSize);
	UInt64 inProcessed = 0;
	int isMT = false;
    auto res = Lzma2DecMt_Decode(decoder, prop, &props, &outStream.vt, &outBufferSize, 1, &inStream.vt,
                                 &inProcessed, &isMT, nullptr);
	
	return res;
}

SRes LzmaUtil::EncodeFile(std::string inFile, std::string outFile,LZMA_DATASIZE* inputFileSize,bool writeheader, bool append /* = false */) {
	CFileSeqInStream inStream;
	CFileOutStream outStream;

	FileSeqInStream_CreateVTable(&inStream);
	File_Construct(&inStream.file);

	FileOutStream_CreateVTable(&outStream);
	File_Construct(&outStream.file);

	if (InFile_Open(&inStream.file, inFile.c_str()) != 0)
	{
		return LZMA_STATUS_OPEN_FILE_FAIL;
	}

	if (MyOpenWirteFile(&outStream.file, outFile.c_str(), append) != 0)
	{
		return LZMA_STATUS_OPEN_FILE_FAIL;
	}
	UInt64 fileSize;
	File_GetLength(&inStream.file, &fileSize);
	*inputFileSize = fileSize;

	CLzma2EncHandle encoder = Lzma2Enc_Create(&util_g_Alloc, &util_g_Alloc);
	// set up properties
	CLzma2EncProps props;
	Lzma2EncProps_Init(&props);
	props.lzmaProps.dictSize = fileSize;
	props.lzmaProps.level  = LZMA_LEVEL;
	props.numTotalThreads = LZMA_THREAD_NUM;

	Lzma2Enc_SetProps(encoder, &props);

	//Lzma2Enc_SetDataSize(&props,inputSize);
	if(writeheader){
		Byte header[1 + 8];
		int i;

		header[0] = Lzma2Enc_WriteProperties(encoder);
		for (i = 0; i < 8; i++)
			header[i+1] = (Byte)(fileSize >> (8 * i));
		
		const Byte* headerdata = const_cast<const Byte*>(header);
		size_t header_size = 9;
		File_Write(&outStream.file,(const void*)headerdata,&header_size);
	}
	LZMA_DATASIZE outBufSize = 0;
	auto res = Lzma2Enc_Encode2(encoder,&outStream.vt,NULL,&outBufSize,&inStream.vt,NULL,fileSize,NULL);

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

SRes LzmaUtil::EncodeFileToByte(std::string inFile, LZMA_DATASIZE* inputFileSize, mc::ByteStream* bytesteam,bool writeheader) {

	CByteSeqOutStream outStream;
	outStream.vt.Write = MyByteWrite;
	outStream.bytesteam = bytesteam;


	CFileSeqInStream inStream;

	FileSeqInStream_CreateVTable(&inStream);
	File_Construct(&inStream.file);

	if (InFile_Open(&inStream.file, inFile.c_str()) != 0)
	{
		return LZMA_STATUS_OPEN_FILE_FAIL;
	}

	UInt64 fileSize;
	File_GetLength(&inStream.file, &fileSize);
	*inputFileSize = fileSize;

	CLzma2EncHandle encoder = Lzma2Enc_Create(&util_g_Alloc, &util_g_Alloc);
	// set up properties
	CLzma2EncProps props;
	Lzma2EncProps_Init(&props);
	props.lzmaProps.dictSize = fileSize;
	props.lzmaProps.level  = LZMA_LEVEL;
	props.numTotalThreads = LZMA_THREAD_NUM;

	Lzma2Enc_SetProps(encoder, &props);

	//Lzma2Enc_SetDataSize(&props,inputSize);
	if(writeheader){
		Byte header[1 + 8];
		int i;

		header[0] = Lzma2Enc_WriteProperties(encoder);
		for (i = 0; i < 8; i++)
			header[i+1] = (Byte)(fileSize >> (8 * i));
		
		bytesteam->write((char*)header,9);
	}
	LZMA_DATASIZE outBufSize = 0;
	auto res = Lzma2Enc_Encode2(encoder,&outStream.vt,NULL,&outBufSize,&inStream.vt,NULL,fileSize,NULL);
	
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