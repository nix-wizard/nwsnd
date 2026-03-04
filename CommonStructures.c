/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <string.h>

#include "CommonStructures.h"

#define ALLOCATE(POINTER, SIZE) POINTER = malloc(SIZE); addPointerToPointerList(POINTER, pointerList);

u32
readLittleEndian(FILE *file, u32 bytes)
{
	u32 value = 0;
	u32 bits = bytes * 8;
	for (s32 i = 0; i < bits; i += 8) {
		value |= fgetc(file) << i;
	}
	return value;
}

u32
readBigEndian(FILE *file, u32 bytes)
{
	u32 value = 0;
	u32 bits = bytes * 8;
	for (s32 i = bits - 8; i >= 0; i -= 8) {
		value |= fgetc(file) << i;
	}
	return value;
}

f32
readFloat(FILE *file, u32 (*readBytes)(FILE *, u32))
{
	u32 raw = readBytes(file, 4);
	f32 value;
	memcpy(&value, &raw, sizeof(f32));
	return value;
}

Status
setEndianess(u16 byteOrder, u32 (**readBytesPointer)(FILE *, u32))
{
	if (byteOrder == 0xFFFE) {
		*readBytesPointer = readLittleEndian;
	}
	else if (byteOrder == 0xFEFF) {
		*readBytesPointer = readBigEndian;
	}
	else {
		fprintf(stderr, "Invalid byte order marker.\n");
		return STATUS_ERR;
	}

	return STATUS_OK;
}

void
addPointerToPointerList(void *pointer, struct PointerList *pointerList)
{
	u32 newCount = pointerList->count + 1;
	if (pointerList->count != 0) {
		pointerList->pointers = realloc(pointerList->pointers, newCount*sizeof(void *));
	}
	else {
		pointerList->pointers = malloc(sizeof(void *));
	}
	pointerList->pointers[pointerList->count] = pointer;
	pointerList->count = newCount;
}

void
freePointerList(struct PointerList pointerList)
{
	for (u32 i = 0; i < pointerList.count; i += 1) {
		free(pointerList.pointers[i]);
	}
	free(pointerList.pointers);
}

Status
readFileHeader(struct FileHeader *fileHeader, FILE *file, char fileType[4], u32 (**readBytesPointer)(FILE *, u32))
{
	fileHeader->filePosition = ftell(file);
	fread(fileHeader->fileType, 1, 4, file);
	fileHeader->fileType[4] = '\0';
	if (strncmp(fileHeader->fileType, fileType, 4) != 0) {
		fprintf(stderr, "File type mismatch in header. Likely invalid file.\n");
		return STATUS_ERR;
	}
	fileHeader->byteOrder = readBigEndian(file, 2);
	if (setEndianess(fileHeader->byteOrder, readBytesPointer) != STATUS_OK) {
		fprintf(stderr, "Invalid byte order marker.\n");
		return STATUS_ERR;
	}
	u32 (*readBytes)(FILE *file, u32 bytes) = (*readBytesPointer);
	fileHeader->headerLength = readBytes(file, 2);
	fileHeader->fileVersion = readBytes(file, 4);
	fileHeader->fileLength = readBytes(file, 4);
	fileHeader->partitionCount = readBytes(file, 2);
	fseek(file, 2, SEEK_CUR);

	return STATUS_OK;
}

Status
readPartitionHeader(struct PartitionHeader *partitionHeader, FILE *file, char partitionType[4], u32 (*readBytes)(FILE *file, u32 bytes))
{
	partitionHeader->filePosition = ftell(file);
	fread(partitionHeader->partitionType, 1, 4, file);
	partitionHeader->partitionType[4] = '\0';
	if (strncmp(partitionHeader->partitionType, partitionType, 4) != 0) {
		fprintf(stderr, "Partition header type mismatch.\n");
		return STATUS_ERR;
	}
	partitionHeader->length = readBytes(file, 2);
	fseek(file, 2, SEEK_CUR);

	return STATUS_OK;
}

Status
readLink(struct Link *link, FILE *file, u32 (*readBytes)(FILE *file, u32 bytes))
{
	link->filePosition = ftell(file);
	link->referenceID = readBytes(file, 2);
	fseek(file, 2, SEEK_CUR);
	link->offset = readBytes(file, 4);
	return STATUS_OK;
}

Status
readLinkWithLength(struct LinkWithLength *link, FILE *file, u32 (*readBytes)(FILE *file, u32 bytes))
{
	link->filePosition = ftell(file);
	link->referenceID = readBytes(file, 2);
	fseek(file, 2, SEEK_CUR);
	link->offset = readBytes(file, 4);
	link->length = readBytes(file,4);
	
	return STATUS_OK;
}

Status
readLinkTable(struct LinkTable *linkTable, FILE *file, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	linkTable->filePosition = ftell(file);
	linkTable->count = readBytes(file, 4);
	ALLOCATE(linkTable->table, sizeof(struct Link) * linkTable->count)
	for (u32 i = 0; i < linkTable->count; i += 1) {
		if (readLink(&linkTable->table[i], file, readBytes) != STATUS_OK) {
			fprintf(stderr, "Invalid link in link table.\n");
			return STATUS_ERR;
		}
	}

	return STATUS_OK;
}

Status
readLinkWithLengthTable(struct LinkWithLengthTable *linkTable, FILE *file, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	linkTable->filePosition = ftell(file);
	linkTable->count = readBytes(file, 4);
	ALLOCATE(linkTable->table, sizeof(struct LinkWithLength) * linkTable->count)
	for (u32 i = 0; i < linkTable->count; i += 1) {
		if (readLinkWithLength(&linkTable->table[i], file, readBytes) != STATUS_OK) {
			fprintf(stderr, "Invalid link in link with length table.\n");
			return STATUS_ERR;
		}
		linkTable->size += linkTable->table[i].length;
	}

	return STATUS_OK;
}

Status
getLinkByReferenceID(u32 *out, u32 linkCount, struct Link links[linkCount], ReferenceID referenceID)
{
	for (u32 i = 0; i < linkCount; i++) {
		if (links[i].referenceID == referenceID) {
			*out = i;
			return STATUS_OK;
		}
	}
	
	fprintf(stderr, "Failed to find a link of that type.\n");
	return STATUS_ERR;
}

Status
getLinkWithLengthByReferenceID(u32 *out, u32 linkCount, struct LinkWithLength links[linkCount], ReferenceID referenceID)
{
	for (u32 i = 0; i < linkCount; i++) {
		if (links[i].referenceID == referenceID) {
			*out = i;
			return STATUS_OK;
		}
	}

	fprintf(stderr, "Failed to find a link of that type.\n");
	return STATUS_ERR;
}

u32
getBitFlagParameterIndex(u32 bitFlag, u32 bitNumber)
{
	if (bitNumber > 31) {
		return STATUS_ERR;
	}

	u8 valid = 0;
	u32 count = 0;
	for (u32 i = 0; i <= bitNumber; i++) {
		if (bitFlag & (0x1 << i)) {
			count += 1;
			if (i == bitNumber) {
				return count;
			}
		}
	}
	return FALSE;
}

Status
readOptionParameter(struct OptionParameter *optionParameter, FILE *file, u32 (*readBytes)(FILE *file, u32 bytes))
{
	optionParameter->filePosition = ftell(file);
	optionParameter->bitFlag = readBytes(file, 4);

	return STATUS_OK;
}

u8
getByte(u32 value, u8 index)
{
	return (u8)((value >> 8 * index) & 0xFF);
}

Status
readU8Table(struct U8Table *u8Table, FILE *file, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	u8Table->filePosition = ftell(file);
	u8Table->count = readBytes(file, 4);
	ALLOCATE(u8Table->table, sizeof(u8) * u8Table->count)
	for (u32 i = 0; i < u8Table->count; i += 1) {
		u8Table->table[i] = readBytes(file, 1);
	}

	return STATUS_OK;
}

Status
readU32Table(struct U32Table *u32Table, FILE *file, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	u32Table->filePosition = ftell(file);
	u32Table->count = readBytes(file, 4);
	ALLOCATE(u32Table->table, sizeof(u32) * u32Table->count)
	for (u32 i = 0; i < u32Table->count; i += 1) {
		u32Table->table[i] = readBytes(file, 4);
	}

	return STATUS_OK;
}