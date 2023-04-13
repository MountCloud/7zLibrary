include_directories(${CMAKE_CURRENT_LIST_DIR})

set(7Z_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/7z/7zAlloc.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/7zArcIn.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/7zBuf.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/7zBuf2.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/7zCrc.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/7zCrcOpt.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/7zDec.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/7zFile.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/7zStream.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Aes.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/AesOpt.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Alloc.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Bcj2.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Bcj2Enc.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Bra.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Bra86.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/BraIA64.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/CpuArch.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Delta.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/DllSecur.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/LzFind.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Lzma2Dec.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Lzma2DecMt.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Lzma2Enc.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Lzma86Dec.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Lzma86Enc.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/LzmaDec.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/LzmaEnc.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/LzmaLib.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Ppmd7.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Ppmd7Dec.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Ppmd7Enc.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Sha256.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Sort.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/Xz.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/XzCrc64.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/XzCrc64Opt.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/XzDec.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/XzEnc.c
    ${CMAKE_CURRENT_LIST_DIR}/7z/XzIn.c
    ${CMAKE_CURRENT_LIST_DIR}/util/lzmautil.cpp
    ${CMAKE_CURRENT_LIST_DIR}/util/io/bytestream.cpp
)