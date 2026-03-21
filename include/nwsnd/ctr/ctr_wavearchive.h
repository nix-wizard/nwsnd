/* See LICENSE file for copyright and license details. */
#ifndef NWSND_CTR_WAVEARCHIVE_H
#define NWSND_CTR_WAVEARCHIVE_H

#include "nwsnd/common.h"

typedef struct {
	struct PointerList pointerList;

	u32 filePosition;
	struct CTR_WaveArchive_FileHeader {
		u32 filePosition;
		struct FileHeader fileHeader;
		struct LinkWithLength partitionLinks[2];
	} header;

	struct CTR_WaveArchive_InfoPartition {
		struct {
			u32 filePosition;
			struct PartitionHeader partitionHeader;
		} header;

		struct {
			u32 filePosition;
			struct LinkWithLengthTable table;
		 } body;
	} infoPartition;

	struct CTR_WaveArchive_FilePartition {
		struct {
			u32 filePosition;
			struct PartitionHeader partitionHeader;
		} header;
		
		struct {
			u32 filePosition;	
		} body;

		char **files;
	} filePartition;
} CTR_WaveArchive;

Status
readCTR_WaveArchive_FileHeader(struct CTR_WaveArchive_FileHeader *header, FILE *waveArchiveFile, u32 (**readBytesPointer)(FILE *, u32));

Status
readCTR_WaveArchive_InfoPartition(struct CTR_WaveArchive_InfoPartition *infoPartition, FILE *waveArchiveFile, struct PointerList *pointerList, u32 (*readBytes)(FILE *, u32));

Status
readCTR_WaveArchive_FilePartition(struct CTR_WaveArchive_FilePartition *filePartition, FILE *waveArchiveFile, struct PointerList *pointerList, struct CTR_WaveArchive_InfoPartition *infoPartition, u32 (*readBytes)(FILE *, u32));

Status
readCTR_WaveArchive(CTR_WaveArchive *waveArchive, FILE *waveArchiveFile);

#endif