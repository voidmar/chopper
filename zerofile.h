// zerofile.h
// Copyright (C) 2020-2021 The Leading Zeros, Inc. All rights reserved.
//
// RIFF file format for parasitic extraction

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#include "debug.h"

#pragma once

#define ZEROFILE_VERSION_MAJOR 0
#define ZEROFILE_VERSION_MINOR 1
#define ZEROFILE_VERSION_BUILD 0

// RIFF files are chunk based. A chunk starts with 32-bit, 4-character identifier, then 32-bits
// describing the length of the payload (not including the two fields)
// 
// Only RIFF header and LIST chunks can contain other chunks, and they identify what to expect
// using an additional 4-byte "type" field.
// 
// ZERO files are structured similarly to a TIFF or WAV file, a RIFF header tagged as "ZERO"
// followed by as many chunks of these 4 types as needed:
//
//     "RIFF"
//     <file_length>
//     "ZERO"       <- This file contains a parasitic extraction
//         "SPLI"   <- spline parameters
//            ...
//         "CBUF"   <- command buffer bytes
//            ...
//         "CPLX"   <- complex value per pin
//            ...
//         "PVOS"   <- 8-bit volume extraction in scanline order
//            ...
//         "PVOL"   <- 8-bit volume extraction in tiled order
//            ...
// 
// Example output from "zerotool":
// 
//      chunk ID : "RIFF"
//      size : 2949184
//      type : "ZERO"
//          chunk ID : "SPLI"
//          size : 16
//              file_version : 0.1.0
//              width : 640
//              height : 1536
//              hpitch : 37236cd9 = 9.7409 um
//          chunk ID : "PVOL"
//          size : 983044
//              lambda : bdd8 = 630 nm
//              gain : 65535
//              22 4c c2 92 3d 8d 48 d1 e4 d7 8c 29 03 79 64 e7
//              01 47 e4 95 09 15 80 d4 f9 2e 58 03 c8 ea bf 56
//              ...
// 
// RIFF chunk API philosophy
// -------------------------
// - When reading a RIFF file, after visiting a chunk your read options are constrained to bytes inside that chunk.
// - You are free to read, seek and jump around inside that chunk but reading outside of it will raise an error.
// - To access the rest of the file you request the "next chunk" or search for chunk by specific ID.
// - If the high-level API doesn't work for your use case, direct low level access to RIFF file chunks is available.
// 
// Zerofile API philosophy
// -----------------------
// - Zerofiles are built on RIFF chunks with rules.
// - The API will never allocate memory, working only on storage that has been passed in to functions.
// - The first chunk in a file MUST be the "ZERO" chunk because that contains extraction parameters
// - Extraction parameters are used to calculate the size of subsequent payloads.
// - Opening a Zerofile will immediately parse and extract the "ZERO" records.
// - The high level API is built to assume it contains a single extraction
//     - One device
//     - One speaker
//     - One envelope
//     - Several fields per envelope
// - As we need to extend these rules we will bump the version and add "LIST" elements to each of these records.
//
// - There are many opportunities for compression, let's discuss.


// error codes -----------------------------------------------------------------

enum Zerofile_ErrorCode {
