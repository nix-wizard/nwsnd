/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>

#include "nwsnd/ctr/ctr_wavearchive.h"

Status
readCTR_WaveArchive_FileHeader(struct CTR_WaveArchive_FileHeader *header, FILE *waveArchiveFile, u32 (**readBytesPointer)(FILE *, u32))
{
	header->filePosition = ftell(waveArchiveFile);
	CATCH(readFileHeader(&header->fileHeader, waveArchiveFile, "CWAR", readBytesPointer) != STATUS_OK, "file header", "wave archive header")

	u32 (*readBytes)(FILE *file, u32 bytes) = (*readBytesPointer);

	for (u32 i = 0; i < 2; i += 1) {
		CATCH(readLinkWithLength(&header->partitionLinks[i], waveArchiveFile, readBytes) != STATUS_OK, "link", "wave archive header")
	}

	return STATUS_OK;
}

Status
readCTR_WaveArchive_InfoPartition(struct CTR_WaveArchive_InfoPartition *infoPartition, FILE *waveArchiveFile, struct PointerList *pointerList, u32 (*readBytes)(FILE *, u32)) {
	infoPartition->header.filePosition = ftell(waveArchiveFile);
	CATCH(readPartitionHeader(&infoPartition->header.partitionHeader, waveArchiveFile, "INFO", readBytes) != STATUS_OK, "partition header", "wave archive info partition")

	infoPartition->body.filePosition = ftell(waveArchiveFile);
	CATCH(readLinkWithLengthTable(&infoPartition->body.table, waveArchiveFile, readBytes, pointerList) != STATUS_OK, "link table", "wave archive info partition")

	return STATUS_OK;
}

Status
readCTR_WaveArchive_FilePartition(struct CTR_WaveArchive_FilePartition *filePartition, FILE *waveArchiveFile, struct PointerList *pointerList, struct CTR_WaveArchive_InfoPartition *infoPartition, u32 (*readBytes)(FILE *, u32))
{
	filePartition->header.filePosition = ftell(waveArchiveFile);
	CATCH(readPartitionHeader(&filePartition->header.partitionHeader, waveArchiveFile, "FILE", readBytes) != STATUS_OK, "partition header", "wave archive file partition")

	filePartition->body.filePosition = ftell(waveArchiveFile);
	
	ALLOCATE(filePartition->files, sizeof(char *) * infoPartition->body.table.count)
	for (u32 i = 0; i < infoPartition->body.table.count; i += 1) {
		ALLOCATE(filePartition->files[i], infoPartition->body.table.table[i].length)
		fseek(waveArchiveFile, filePartition->body.filePosition + infoPartition->body.table.table[i].offset, SEEK_SET);
		fread(filePartition->files[i], 1, infoPartition->body.table.table[i].length, waveArchiveFile);
	}

	return STATUS_OK;
}

Status
readCTR_WaveArchive(CTR_WaveArchive *waveArchive, FILE *waveArchiveFile)
{
	printf("Reading wave archive...\n");
	waveArchive->filePosition = ftell(waveArchiveFile);

	u32 (*readBytes)(FILE *file, u32 bytes);

	/* Set pointer list count */
	waveArchive->pointerList.count = 0;

	/* Header */
	printf("Reading wave archive file header...\n");
	CATCH(readCTR_WaveArchive_FileHeader(&waveArchive->header, waveArchiveFile, &readBytes) != STATUS_OK, "file header", "wave archive file")
	printf("File header read.\n");

	/* Info Partition */
	printf("Reading wave archive info partition...\n");
	fseek(waveArchiveFile, waveArchive->header.partitionLinks[0].offset, SEEK_SET);
	CATCH(readCTR_WaveArchive_InfoPartition(&waveArchive->infoPartition, waveArchiveFile, &waveArchive->pointerList, readBytes) != STATUS_OK, "info partition", "wave archive file")
	printf("Info partition read.\n");

	/* File Partition */
	printf("Reading wave archive file partition...\n");
	fseek(waveArchiveFile, waveArchive->header.partitionLinks[1].offset, SEEK_SET);
	CATCH(readCTR_WaveArchive_FilePartition(&waveArchive->filePartition, waveArchiveFile, &waveArchive->pointerList, &waveArchive->infoPartition, readBytes) != STATUS_OK, "info partition", "wave archive file")
	printf("File partition read.\n");

	printf("Sound archive read!");
	return STATUS_OK;
}