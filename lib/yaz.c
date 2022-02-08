#include "yaz.h"

// decoder implementation by thakis of http://www.amnoid.de
// encoder implementation by shevious, with bug fixes by notwa

#define MAX_RUNLEN (0xFF + 0x12)

// simple and straight encoding scheme for Yaz0
static u32 Yaz_SimpleEncode(u8* src, s32 size, s32 pos, u32* pMatchPos) {
	s32 numBytes = 1;
	s32 matchPos = 0;
	
	s32 startPos = pos - 0x1000;
	s32 end = size - pos;
	
	if (startPos < 0)
		startPos = 0;
	
	// maximum runlength for 3 byte encoding
	if (end > MAX_RUNLEN)
		end = MAX_RUNLEN;
	
	for (s32 i = startPos; i < pos; i++) {
		s32 j;
		
		for (j = 0; j < end; j++) {
			if (src[i + j] != src[j + pos])
				break;
		}
		if (j > numBytes) {
			numBytes = j;
			matchPos = i;
		}
	}
	
	*pMatchPos = matchPos;
	
	if (numBytes == 2)
		numBytes = 1;
	
	return numBytes;
}

// a lookahead encoding scheme for ngc Yaz0
static u32 Yaz_NintendoEncode(u8* src, s32 size, s32 pos, u32* pMatchPos) {
	u32 numBytes = 1;
	static u32 numBytes1;
	static u32 matchPos;
	static s32 prevFlag = 0;
	
	// if prevFlag is set, it means that the previous position
	// was determined by look-ahead try.
	// so just use it. this is not the best optimization,
	// but nintendo's choice for speed.
	if (prevFlag == 1) {
		*pMatchPos = matchPos;
		prevFlag = 0;
		
		return numBytes1;
	}
	
	prevFlag = 0;
	numBytes = Yaz_SimpleEncode(src, size, pos, &matchPos);
	*pMatchPos = matchPos;
	
	// if this position is RLE encoded, then compare to copying 1 byte and next position(pos+1) encoding
	if (numBytes >= 3) {
		numBytes1 = Yaz_SimpleEncode(src, size, pos + 1, &matchPos);
		// if the next position encoding is +2 longer than current position, choose it.
		// this does not guarantee the best optimization, but fairly good optimization with speed.
		if (numBytes1 >= numBytes + 2) {
			numBytes = 1;
			prevFlag = 1;
		}
	}
	
	return numBytes;
}

// src points to the yaz0 source data (to the "real" source data, not at the header!)
// dst points to a buffer uncompressedSize bytes large (you get uncompressedSize from
// the second 4 bytes in the Yaz0 header).
void Yaz_Decode(u8* src, u8* dst, s32 uncompressedSize) {
	s32 srcPlace = 0, dstPlace = 0; // current read/write positions
	
	u32 validBitCount = 0; // number of valid bits left in "code" byte
	u8 currCodeByte;
	
	while (dstPlace < uncompressedSize) {
		// read new "code" byte if the current one is used up
		if (validBitCount == 0) {
			currCodeByte = src[srcPlace];
			++srcPlace;
			validBitCount = 8;
		}
		
		if ((currCodeByte & 0x80) != 0) {
			// straight copy
			dst[dstPlace] = src[srcPlace];
			dstPlace++;
			srcPlace++;
		} else {
			// RLE part
			u8 byte1 = src[srcPlace];
			u8 byte2 = src[srcPlace + 1];
			srcPlace += 2;
			
			u32 dist = ((byte1 & 0xF) << 8) | byte2;
			u32 copySource = dstPlace - (dist + 1);
			
			u32 numBytes = byte1 >> 4;
			if (numBytes == 0) {
				numBytes = src[srcPlace] + 0x12;
				srcPlace++;
			} else {
				numBytes += 2;
			}
			
			// copy run
			for (u32 i = 0; i < numBytes; ++i) {
				dst[dstPlace] = dst[copySource];
				copySource++;
				dstPlace++;
			}
		}
		
		// use next bit from "code" byte
		currCodeByte <<= 1;
		validBitCount -= 1;
	}
}

s32 Yaz_Encode(u8* src, u8* dst, s32 srcSize) {
	s32 srcPos = 0;
	s32 dstPos = 0;
	s32 bufPos = 0;
	
	u8 buf[24]; // 8 codes * 3 bytes maximum
	
	u32 validBitCount = 0; // number of valid bits left in "code" byte
	u8 currCodeByte = 0; // a bitfield, set bits meaning copy, unset meaning RLE
	
	while (srcPos < srcSize) {
		u32 numBytes;
		u32 matchPos;
		
		numBytes = Yaz_NintendoEncode(src, srcSize, srcPos, &matchPos);
		if (numBytes < 3) {
			// straight copy
			buf[bufPos] = src[srcPos];
			bufPos++;
			srcPos++;
			//set flag for straight copy
			currCodeByte |= (0x80 >> validBitCount);
		} else {
			//RLE part
			u32 dist = srcPos - matchPos - 1;
			u8 byte1, byte2, byte3;
			
			if (numBytes >= 0x12) { // 3 byte encoding
				byte1 = 0 | (dist >> 8);
				byte2 = dist & 0xFF;
				buf[bufPos++] = byte1;
				buf[bufPos++] = byte2;
				// maximum runlength for 3 byte encoding
				if (numBytes > MAX_RUNLEN)
					numBytes = MAX_RUNLEN;
				byte3 = numBytes - 0x12;
				buf[bufPos++] = byte3;
			} else { // 2 byte encoding
				byte1 = ((numBytes - 2) << 4) | (dist >> 8);
				byte2 = dist & 0xFF;
				buf[bufPos++] = byte1;
				buf[bufPos++] = byte2;
			}
			srcPos += numBytes;
		}
		
		validBitCount++;
		
		// write eight codes
		if (validBitCount == 8) {
			dst[dstPos++] = currCodeByte;
			for (s32 j = 0; j < bufPos; j++)
				dst[dstPos++] = buf[j];
			
			currCodeByte = 0;
			validBitCount = 0;
			bufPos = 0;
		}
	}
	
	if (validBitCount > 0) {
		dst[dstPos++] = currCodeByte;
		for (s32 j = 0; j < bufPos; j++)
			dst[dstPos++] = buf[j];
		
		currCodeByte = 0;
		validBitCount = 0;
		bufPos = 0;
	}
	
	return dstPos;
}
