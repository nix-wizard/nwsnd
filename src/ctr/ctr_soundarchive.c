/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>

#include "nwsnd/ctr/ctr_soundarchive.h"

#define BITFLAG(OPTIONPARAMETER) \
	u32 currentIndex; \
	for (u32 i = 0; i < sizeof(optionParams)/sizeof(optionParams[0]); i += 1) { \
		currentIndex = getBitFlagParameterIndex(OPTIONPARAMETER.bitFlag, optionParams[i].bitIndex); \
		if (currentIndex != FALSE) { \
			fseek(soundArchiveFile, OPTIONPARAMETER.filePosition + (currentIndex * 4), SEEK_SET); \
			*optionParams[i].destination = readBytes(soundArchiveFile, 4); \
		} \
	}

#define INFO(TABLEINDEX, LINKTABLE, INFO, TYPE, READER) \
	fseek(soundArchiveFile, infoPartition->body.filePosition + infoPartition->body.tableLinks[TABLEINDEX].offset, SEEK_SET); \
	readLinkTable(&infoPartition->body.LINKTABLE, soundArchiveFile, readBytes, pointerList); \
	ALLOCATE(infoPartition->body.INFO, sizeof(TYPE) * infoPartition->body.LINKTABLE.count) \
	for (u32 i = 0; i < infoPartition->body.LINKTABLE.count; i += 1) { \
		fseek(soundArchiveFile, infoPartition->body.LINKTABLE.filePosition + infoPartition->body.LINKTABLE.table[i].offset, SEEK_SET); \
		CATCH(READER(&infoPartition->body.INFO[i], soundArchiveFile, readBytes, pointerList) != STATUS_OK, "info index TABLEINDEX", "info partition") \
	}

Status
readCTRItemID(struct CTRItemID *itemID, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes))
{
	itemID->filePosition = ftell(soundArchiveFile);
	itemID->ID = readBytes(soundArchiveFile, 3);
	itemID->type = readBytes(soundArchiveFile, 1);

	return STATUS_OK;
};

Status
readCTRItemIDTable(struct CTRItemIDTable *itemIDTable, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	itemIDTable->filePosition = ftell(soundArchiveFile);
	itemIDTable->count = readBytes(soundArchiveFile, 4);
	ALLOCATE(itemIDTable->table, sizeof(struct CTRItemID) * itemIDTable->count)
	for (u32 i = 0; i < itemIDTable->count; i += 1) {
		CATCH(readCTRItemID(&itemIDTable->table[i], soundArchiveFile, readBytes) != STATUS_OK, "item ID", "item ID table")
	}

	return STATUS_OK;
};

Status
readCTRSendValue(struct CTRSendValue *sendValue, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes))
{
	sendValue->filePosition = ftell(soundArchiveFile);
	sendValue->mainSend = readBytes(soundArchiveFile, 1);
	sendValue->fxSend[1] = readBytes(soundArchiveFile, 1);
	sendValue->fxSend[2] = readBytes(soundArchiveFile, 1);
	fseek(soundArchiveFile, 1, SEEK_CUR);

	return STATUS_OK;
}

Status
readCTRSoundArchiveHeader(struct CTRSoundArchiveHeader *header, FILE *soundArchiveFile, u32 (**readBytesPointer)(FILE *, u32))
{
	header->filePosition = ftell(soundArchiveFile);
	CATCH(readFileHeader(&header->fileHeader, soundArchiveFile, "CSAR", readBytesPointer) != STATUS_OK, "file header", "file")

	u32 (*readBytes)(FILE *file, u32 bytes) = (*readBytesPointer);

	for (u32 i = 0; i < 3; i++) {
		CATCH(readLinkWithLength(&header->partitionLinks[i], soundArchiveFile, readBytes) != STATUS_OK, "link", "sound archive header")
	}
	
	return STATUS_OK;
}

Status
readCTRNode(struct CTRNode *node, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes))
{
	node->filePosition = ftell(soundArchiveFile);
	node->flags = readBytes(soundArchiveFile, 2);
	node->bitIndex = readBytes(soundArchiveFile, 2);
	node->leftIndex = readBytes(soundArchiveFile, 4);
	node->rightIndex = readBytes(soundArchiveFile, 4);
	node->stringID = readBytes(soundArchiveFile, 4);
	CATCH(readCTRItemID(&node->itemID, soundArchiveFile, readBytes) != STATUS_OK, "item ID", "node table")

	return STATUS_OK;
}

Status
readCTRNodeTable(struct CTRNodeTable *nodeTable, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	nodeTable->filePosition = ftell(soundArchiveFile);
	nodeTable->count = readBytes(soundArchiveFile, 4);

	u32 leafCount = 0;

	ALLOCATE(nodeTable->nodes, sizeof(struct CTRNode) * nodeTable->count)
	for (u32 i = 0; i < nodeTable->count; i += 1) {
		CATCH(readCTRNode(&nodeTable->nodes[i], soundArchiveFile, readBytes) != STATUS_OK, "node", "node table")
		if ((nodeTable->nodes[i].flags & 1) == 1) { /* Is a leaf */
			leafCount += 1;
		}
	}

	ALLOCATE(nodeTable->itemIDToNode, sizeof(struct CTRNode *) * leafCount)
	for (u32 i = 0; i < nodeTable->count; i += 1) {
		if ((nodeTable->nodes[i].flags & 1) == 1) {
			nodeTable->itemIDToNode[nodeTable->nodes[i].itemID.ID] = &nodeTable->nodes[i]; /* TODO: Warn for duplicates */
		}
	}

	return STATUS_OK;
}

Status
readCTRPatriciaTree(struct CTRPatriciaTree *patriciaTree, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	patriciaTree->filePosition = ftell(soundArchiveFile);
	patriciaTree->rootIndex = readBytes(soundArchiveFile, 4);
	CATCH(readCTRNodeTable(&patriciaTree->nodeTable, soundArchiveFile, readBytes, pointerList) != STATUS_OK, "node table", "patricia tree")

	return STATUS_OK;
}

Status
readCTRSoundArchiveStringPartition(struct CTRSoundArchiveStringPartition *stringPartition, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	/* Header */
	stringPartition->header.filePosition = ftell(soundArchiveFile);
	CATCH(readPartitionHeader(&stringPartition->header.partitionHeader, soundArchiveFile, "STRG", readBytes) != STATUS_OK, "header", "string partition")

	/* Body */
	stringPartition->body.filePosition = ftell(soundArchiveFile);
	for (u32 i = 0; i < 2; i++) {
		CATCH(readLink(&stringPartition->body.tableLinks[i], soundArchiveFile, readBytes) != STATUS_OK, "link", "string partition")
	}

	stringPartition->body.filenameLinksOffset = stringPartition->body.filePosition + stringPartition->body.tableLinks[0].offset;
	stringPartition->body.patriciaTreeOffset = stringPartition->body.filePosition + stringPartition->body.tableLinks[1].offset;

	fseek(soundArchiveFile, stringPartition->body.filenameLinksOffset, SEEK_SET);
	u32 filenameTableBase = ftell(soundArchiveFile);

	readLinkWithLengthTable(&stringPartition->body.filenameLinkTable, soundArchiveFile, readBytes, pointerList);

	ALLOCATE(stringPartition->body.filenameTable, stringPartition->body.filenameLinkTable.size)
	for (u32 i = 0; i < stringPartition->body.filenameLinkTable.count; i++) {
		fseek(soundArchiveFile, filenameTableBase + stringPartition->body.filenameLinkTable.table[i].offset, SEEK_SET);
		stringPartition->body.filenameTable[i].filePosition = ftell(soundArchiveFile);
		ALLOCATE(stringPartition->body.filenameTable[i].filename, stringPartition->body.filenameLinkTable.table[i].length)
		fread(stringPartition->body.filenameTable[i].filename, 1, stringPartition->body.filenameLinkTable.table[i].length, soundArchiveFile);
	}

	/* Patricia Tree */
	fseek(soundArchiveFile, stringPartition->body.patriciaTreeOffset, SEEK_SET);
	CATCH(readCTRPatriciaTree(&stringPartition->body.patriciaTree, soundArchiveFile, readBytes, pointerList) != STATUS_OK, "patricia tree", "string partition")

	return STATUS_OK;
}

Status
readCTRSound3DInfo(struct CTRSound3DInfo *sound3DInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes))
{
	sound3DInfo->filePosition = ftell(soundArchiveFile);
	sound3DInfo->flags = readBytes(soundArchiveFile, 4);
	sound3DInfo->decayRatio = readFloat(soundArchiveFile, readBytes);
	sound3DInfo->delayCurve = readBytes(soundArchiveFile, 1);
	sound3DInfo->dopplerFactor = readBytes(soundArchiveFile, 1);
	fseek(soundArchiveFile, 2, SEEK_CUR);
	CATCH(readOptionParameter(&sound3DInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "3D sound info")

	return STATUS_OK;
}

Status
readCTRStreamTrackInfo(struct CTRStreamTrackInfo *streamTrackInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	streamTrackInfo->filePosition = ftell(soundArchiveFile);
	streamTrackInfo->volume = readBytes(soundArchiveFile, 1);
	streamTrackInfo->pan = readBytes(soundArchiveFile, 1);
	streamTrackInfo->span = readBytes(soundArchiveFile, 1);
	streamTrackInfo->flags = readBytes(soundArchiveFile, 1);
	readLink(&streamTrackInfo->toGlobalChannelIndexTable, soundArchiveFile, readBytes);
	readLink(&streamTrackInfo->toSendValue, soundArchiveFile, readBytes);
	streamTrackInfo->lpfFreq = readBytes(soundArchiveFile, 1);
	streamTrackInfo->biquadValue = readBytes(soundArchiveFile, 1);
	streamTrackInfo->biquadValue = readBytes(soundArchiveFile, 1);

	fseek(soundArchiveFile, streamTrackInfo->filePosition + streamTrackInfo->toGlobalChannelIndexTable.offset, SEEK_SET);
	readU8Table(&streamTrackInfo->globalChannelIndexTable, soundArchiveFile, readBytes, pointerList);

	if (streamTrackInfo->toSendValue.referenceID == REFID_SOUNDARCHIVEFILE_SENDINFO) {
		fseek(soundArchiveFile, streamTrackInfo->filePosition + streamTrackInfo->toSendValue.offset, SEEK_SET);
		CATCH(readCTRSendValue(&streamTrackInfo->sendValue, soundArchiveFile, readBytes) != STATUS_OK, "send value", "stream track info")
	}
	
	return STATUS_OK;
}

Status
readCTRStreamSoundExtension(struct CTRStreamSoundExtension *streamSoundExtension, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes))
{
	streamSoundExtension->filePosition = ftell(soundArchiveFile);
	streamSoundExtension->streamTypeInfo = readBytes(soundArchiveFile, 4);
	streamSoundExtension->loopStartFrame = readBytes(soundArchiveFile, 4);
	streamSoundExtension->loopEndFrame = readBytes(soundArchiveFile, 4);

	streamSoundExtension->streamTypeInfoParams.streamType = getByte(streamSoundExtension->streamTypeInfo, 0);
	streamSoundExtension->streamTypeInfoParams.loopFlag = getByte(streamSoundExtension->streamTypeInfo, 1);

	return STATUS_OK;
}

Status
readCTRStreamSoundInfo(struct CTRStreamSoundInfo *streamSoundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	streamSoundInfo->filePosition = ftell(soundArchiveFile);
	streamSoundInfo->allocateTrackFlags = readBytes(soundArchiveFile, 2);
	streamSoundInfo->allocateChannelCount = readBytes(soundArchiveFile, 2);
	CATCH(readLink(&streamSoundInfo->toStreamTrackInfoLinkTable, soundArchiveFile, readBytes) != STATUS_OK, "link", "stream info track info table")
	streamSoundInfo->pitch = readBytes(soundArchiveFile, 4);
	CATCH(readLink(&streamSoundInfo->toSendValue, soundArchiveFile, readBytes) != STATUS_OK, "send value link", "stream info")
	CATCH(readLink(&streamSoundInfo->toStreamSoundExtension, soundArchiveFile, readBytes) != STATUS_OK, "sound extension link", "stream sound info")
	streamSoundInfo->prefetchFileID = readBytes(soundArchiveFile, 4);

	/* Stream track info link table */
	if (streamSoundInfo->toStreamTrackInfoLinkTable.referenceID == REFID_STREAMSOUNDFILE_TRACKINFO) {
		fseek(soundArchiveFile, streamSoundInfo->filePosition + streamSoundInfo->toStreamTrackInfoLinkTable.offset, SEEK_SET);
		CATCH(readLinkTable(&streamSoundInfo->streamTrackInfoLinkTable, soundArchiveFile, readBytes, pointerList) != STATUS_OK, "stream track info link table", "stream sound info")
		ALLOCATE(streamSoundInfo->streamTrackInfo, sizeof(struct CTRStreamTrackInfo) * streamSoundInfo->streamTrackInfoLinkTable.count)
		for (u32 i = 0; i < streamSoundInfo->streamTrackInfoLinkTable.count; i += 1) {
			fseek(soundArchiveFile, streamSoundInfo->streamTrackInfoLinkTable.filePosition + streamSoundInfo->streamTrackInfoLinkTable.table[i].offset, SEEK_SET);
			CATCH(readCTRStreamTrackInfo(&streamSoundInfo->streamTrackInfo[i], soundArchiveFile, readBytes, pointerList) != STATUS_OK, "stream track info", "stream sound info")
		}
	}

	/* Send value */
	if (streamSoundInfo->toSendValue.referenceID == REFID_SOUNDARCHIVEFILE_SENDINFO) {
		fseek(soundArchiveFile, streamSoundInfo->filePosition + streamSoundInfo->toSendValue.offset, SEEK_SET);
		CATCH(readCTRSendValue(&streamSoundInfo->sendValue, soundArchiveFile, readBytes) != STATUS_OK, "send value", "stream sound info")
	}

	if (streamSoundInfo->toStreamSoundExtension.referenceID == REFID_SOUNDARCHIVEFILE_STREAMSOUNDEXTENSIONINFO && streamSoundInfo->toStreamSoundExtension.offset != (s32) 0xffffffff) {
		fseek(soundArchiveFile, streamSoundInfo->filePosition + streamSoundInfo->toStreamSoundExtension.offset, SEEK_SET);
		CATCH(readCTRStreamSoundExtension(&streamSoundInfo->streamSoundExtension, soundArchiveFile, readBytes) != STATUS_OK, "stream sound extension", "stream sound info")
	}

	return STATUS_OK;
}

Status
readCTRWaveSoundInfo(struct CTRWaveSoundInfo *waveSoundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes))
{
	waveSoundInfo->filePosition = ftell(soundArchiveFile);
	waveSoundInfo->index = readBytes(soundArchiveFile, 4);
	waveSoundInfo->allocateTrackCount = readBytes(soundArchiveFile, 4);
	CATCH(readOptionParameter(&waveSoundInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "wave sound info")

	/* Parameters */
	struct BitFlag optionParams[1] = {
		{0x00, &waveSoundInfo->priority}
	};
	BITFLAG(waveSoundInfo->optionParameter)

	/* Priority params */
	waveSoundInfo->priorityParams.priorityChannelPriority = getByte(waveSoundInfo->priority, 0);
	waveSoundInfo->priorityParams.isReleasePriorityFix = getByte(waveSoundInfo->priority, 1);

	return STATUS_OK;
}

Status
readCTRSequenceSoundInfo(struct CTRSequenceSoundInfo *sequenceSoundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	sequenceSoundInfo->filePosition = ftell(soundArchiveFile);
	CATCH(readLink(&sequenceSoundInfo->toBankIDTable, soundArchiveFile, readBytes) != STATUS_OK, "bank ID table link", "sequence sound info")
	sequenceSoundInfo->allocateTrackFlags = readBytes(soundArchiveFile, 4);
	CATCH(readOptionParameter(&sequenceSoundInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "sequence sound info")

	/* Parameters */
	struct BitFlag optionParams[2] = {
		{0x00, &sequenceSoundInfo->startOffset},
		{0x01, &sequenceSoundInfo->priority}
	};
	BITFLAG(sequenceSoundInfo->optionParameter)

	/* Priority params */
	sequenceSoundInfo->priorityParams.priorityChannelPriority = getByte(sequenceSoundInfo->priority, 0);
	sequenceSoundInfo->priorityParams.isReleasePriorityFix = getByte(sequenceSoundInfo->priority, 1);

	fseek(soundArchiveFile, sequenceSoundInfo->filePosition + sequenceSoundInfo->toBankIDTable.offset, SEEK_SET);
	CATCH(readU32Table(&sequenceSoundInfo->bankIDTable, soundArchiveFile, readBytes, pointerList) != STATUS_OK, "bank ID table", "sequence sound info")

	return STATUS_OK;
}

Status
readCTRSoundInfo(struct CTRSoundInfo *soundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	soundInfo->filePosition = ftell(soundArchiveFile);
	soundInfo->fileID = readBytes(soundArchiveFile, 4);
	readCTRItemID(&soundInfo->playerID, soundArchiveFile, readBytes);
	soundInfo->volume = readBytes(soundArchiveFile, 1);
	fseek(soundArchiveFile, 3, SEEK_CUR);
	CATCH(readLink(&soundInfo->extraInfoLink, soundArchiveFile, readBytes) != STATUS_OK, "extra info link", "sound info")
	
	CATCH(readOptionParameter(&soundInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "sound info")

	/* Parameters */
	struct BitFlag optionParams[12] = {
		{0x00, &soundInfo->stringID},
		{0x01, &soundInfo->panParam},
		{0x02, &soundInfo->playerParam},
		{0x08, &soundInfo->offsetTo3DParam},
		{0x09, &soundInfo->offsetToSendParam},
		{0x0A, &soundInfo->offsetToModParam},
		{0x10, &soundInfo->offsetToRVLParam},
		{0x11, &soundInfo->offsetToCTRParam},
		{0x1c, &soundInfo->userParam3},
		{0x1d, &soundInfo->userParam2},
		{0x1e, &soundInfo->userParam1},
		{0x0f, &soundInfo->userParam0}
	};
	BITFLAG(soundInfo->optionParameter)

	/* Pan param */
	soundInfo->panParams.panMode = getByte(soundInfo->panParam, 0);
	soundInfo->panParams.panCurve = getByte(soundInfo->panParam, 1);

	/* Player param */
	soundInfo->playerParams.playerPriority = getByte(soundInfo->playerParam, 0);
	soundInfo->playerParams.playerID = getByte(soundInfo->playerParam, 1);

	/* Front Bypass */
	soundInfo->isFrontBypass = FALSE;
	if (getBitFlagParameterIndex(soundInfo->optionParameter.bitFlag, 0x11) != FALSE && soundInfo->offsetToCTRParam != FALSE) {
		soundInfo->isFrontBypass = TRUE;
	}

	/* 3D sound info */
	if (getBitFlagParameterIndex(soundInfo->optionParameter.bitFlag, 0x08) != FALSE) {
		fseek(soundArchiveFile, soundInfo->filePosition + soundInfo->offsetTo3DParam, SEEK_SET);
		CATCH(readCTRSound3DInfo(&soundInfo->sound3DInfo, soundArchiveFile, readBytes) != STATUS_OK, "3D sound info", "sound info")
	}

	/* Extra info */
	switch (soundInfo->extraInfoLink.referenceID) {
	case REFID_SOUNDARCHIVEFILE_STREAMSOUNDINFO:
		fseek(soundArchiveFile, soundInfo->filePosition + soundInfo->extraInfoLink.offset, SEEK_SET);
		CATCH(readCTRStreamSoundInfo(&soundInfo->streamSoundInfo, soundArchiveFile, readBytes, pointerList) != STATUS_OK, "stream sound extra info", "sound info")
		break;
	case REFID_SOUNDARCHIVEFILE_WAVESOUNDINFO:
		fseek(soundArchiveFile, soundInfo->filePosition + soundInfo->extraInfoLink.offset, SEEK_SET);
		CATCH(readCTRWaveSoundInfo(&soundInfo->waveSoundInfo, soundArchiveFile, readBytes) != STATUS_OK, "wave sound extra info", "sound info")
		break;
	case REFID_SOUNDARCHIVEFILE_SEQUENCESOUNDINFO:
		fseek(soundArchiveFile, soundInfo->filePosition + soundInfo->extraInfoLink.offset, SEEK_SET);
		CATCH(readCTRSequenceSoundInfo(&soundInfo->sequenceSoundInfo, soundArchiveFile, readBytes, pointerList) != STATUS_OK, "sequence extra info", "sound info")
		break;
	default:
		break;
	}
	
	return STATUS_OK;
}

Status
readCTRWaveSoundGroupInfo(struct CTRWaveSoundGroupInfo *waveSoundGroupInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	waveSoundGroupInfo->filePosition = ftell(soundArchiveFile);
	CATCH(readLink(&waveSoundGroupInfo->toWaveArchiveItemIDTable, soundArchiveFile, readBytes) != STATUS_OK, "item ID table link", "wave sound group info")
	CATCH(readOptionParameter(&waveSoundGroupInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "wave sound group info")

	/* Wave archive item ID table */
	fseek(soundArchiveFile, waveSoundGroupInfo->filePosition + waveSoundGroupInfo->toWaveArchiveItemIDTable.offset, SEEK_SET);
	CATCH(readCTRItemIDTable(&waveSoundGroupInfo->waveArchiveIDTable, soundArchiveFile, readBytes, pointerList) != STATUS_OK, "wave archive ID table", "wave sound group info")

	return STATUS_OK;
}

Status
readCTRSoundGroupInfo(struct CTRSoundGroupInfo *soundGroupInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	soundGroupInfo->filePosition = ftell(soundArchiveFile);
	CATCH(readCTRItemID(&soundGroupInfo->startID, soundArchiveFile, readBytes) != STATUS_OK, "start ID", "sound group info")
	CATCH(readCTRItemID(&soundGroupInfo->endID, soundArchiveFile, readBytes) != STATUS_OK, "end ID", "sound group info")
	CATCH(readLink(&soundGroupInfo->toFileIdTable, soundArchiveFile, readBytes) != STATUS_OK, "file ID table link", "sound group info")
	CATCH(readLink(&soundGroupInfo->toWaveSoundGroupInfo, soundArchiveFile, readBytes) != STATUS_OK, "wave sound group info link", "sound group info")
	CATCH(readOptionParameter(&soundGroupInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "sound group info")

	/* Parameters */
	struct BitFlag optionParams[1] = {
		{0x00, &soundGroupInfo->stringID}
	};
	BITFLAG(soundGroupInfo->optionParameter)

	/* File ID table */
	fseek(soundArchiveFile, soundGroupInfo->filePosition + soundGroupInfo->toFileIdTable.offset, SEEK_SET);
	CATCH(readU32Table(&soundGroupInfo->fileIDTable, soundArchiveFile, readBytes, pointerList) != STATUS_OK, "file ID table", "sound group info");

	/* Wave sound group info */
	if (soundGroupInfo->toWaveSoundGroupInfo.referenceID == REFID_SOUNDARCHIVEFILE_WAVESOUNDGROUPINFO) {
		fseek(soundArchiveFile, soundGroupInfo->filePosition + soundGroupInfo->toWaveSoundGroupInfo.offset, SEEK_SET);
		CATCH(readCTRWaveSoundGroupInfo(&soundGroupInfo->waveSoundGroupInfo, soundArchiveFile, readBytes, pointerList) != STATUS_OK, "wave sound group info", "sound group info")
	}

	return STATUS_OK;
}

Status
readCTRBankInfo(struct CTRBankInfo *bankInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	bankInfo->filePosition = ftell(soundArchiveFile);
	bankInfo->fileID = readBytes(soundArchiveFile, 4);
	CATCH(readLink(&bankInfo->toWaveArchiveItemIDTable, soundArchiveFile, readBytes) != STATUS_OK, "item ID table link", "bank info")
	CATCH(readOptionParameter(&bankInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "bank info")

	struct BitFlag optionParams[1] = {
		{0x00, &bankInfo->stringID}
	};
	BITFLAG(bankInfo->optionParameter)

	fseek(soundArchiveFile, bankInfo->filePosition + bankInfo->toWaveArchiveItemIDTable.offset, SEEK_SET);
	CATCH(readCTRItemIDTable(&bankInfo->waveArchiveItemIDTable, soundArchiveFile, readBytes, pointerList) != STATUS_OK, "item ID table", "bank info")

	return STATUS_OK;
}

Status
readCTRWaveArchiveInfo(struct CTRWaveArchiveInfo *waveArchiveInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	waveArchiveInfo->filePosition = ftell(soundArchiveFile);
	waveArchiveInfo->fileID = readBytes(soundArchiveFile, 4);
	waveArchiveInfo->isLoadIndividual = FALSE;
	if (readBytes(soundArchiveFile, 1) != FALSE) {
		waveArchiveInfo->isLoadIndividual = TRUE;
	}
	fseek(soundArchiveFile, 3, SEEK_CUR);
	CATCH(readOptionParameter(&waveArchiveInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "wave archive info")

	struct BitFlag optionParams[2] = {
		{0x00, &waveArchiveInfo->stringID},
		{0x01, &waveArchiveInfo->waveCount}
	};
	BITFLAG(waveArchiveInfo->optionParameter)

	return STATUS_OK;
}

Status
readCTRGroupInfo(struct CTRGroupInfo *groupInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	groupInfo->filePosition = ftell(soundArchiveFile);
	groupInfo->fileID = readBytes(soundArchiveFile, 4);
	CATCH(readOptionParameter(&groupInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "group info")

	struct BitFlag optionParams[1] = {
		{0x00, &groupInfo->stringID}
	};
	BITFLAG(groupInfo->optionParameter)

	return STATUS_OK;
}

Status
readCTRPlayerInfo(struct CTRPlayerInfo *playerInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	playerInfo->filePosition = ftell(soundArchiveFile);
	playerInfo->playableSoundMax = readBytes(soundArchiveFile, 4);
	CATCH(readOptionParameter(&playerInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "player info")
	struct BitFlag optionParams[2] = {
		{0x00, &playerInfo->stringID},
		{0x01, &playerInfo->heapSize}
	};
	BITFLAG(playerInfo->optionParameter)

	return STATUS_OK;
}

Status
readCTRInternalFileLocationInfo(struct CTRInternalFileLocationInfo *internalFileInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes))
{
	internalFileInfo->filePosition = ftell(soundArchiveFile);
	CATCH(readLinkWithLength(&internalFileInfo->toDataFromFilePartitionBody, soundArchiveFile, readBytes) != STATUS_OK, "internal file location data link", "internal file location info")

	return STATUS_OK;
}

Status
readCTRExternalFileLocationInfo(struct CTRExternalFileLocationInfo *externalFileInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes))
{
	externalFileInfo->filePosition = ftell(soundArchiveFile);
	/* TODO: Read path */

	return STATUS_OK;
}

Status
readCTRFileInfo(struct CTRFileInfo *fileInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	fileInfo->filePosition = ftell(soundArchiveFile);
	CATCH(readLink(&fileInfo->toFileLocationInfo, soundArchiveFile, readBytes) != STATUS_OK, "file location info link", "file info")
	CATCH(readOptionParameter(&fileInfo->optionParameter, soundArchiveFile, readBytes) != STATUS_OK, "option parameter", "file info")

	switch (fileInfo->toFileLocationInfo.referenceID) {
	case REFID_SOUNDARCHIVEFILE_INTERNALFILEINFO:
		CATCH(readCTRInternalFileLocationInfo(&fileInfo->internalFileInfo, soundArchiveFile, readBytes) != STATUS_OK, "internal file location info", "file info")
		break;
	case REFID_SOUNDARCHIVEFILE_EXTERNALFILEINFO:
		CATCH(readCTRExternalFileLocationInfo(&fileInfo->externalFileInfo, soundArchiveFile, readBytes) != STATUS_OK, "external file location info", "file info")
		break;
	default:
		break;
	}

	return STATUS_OK;
}

Status
readCTRSoundArchivePlayerInfo(struct CTRSoundArchivePlayerInfo *soundArchivePlayerInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes))
{
	soundArchivePlayerInfo->filePosition = ftell(soundArchiveFile);
	
	soundArchivePlayerInfo->sequenceSoundMax = readBytes(soundArchiveFile, 2);
	soundArchivePlayerInfo->sequenceTrackMax = readBytes(soundArchiveFile, 2);
	soundArchivePlayerInfo->streamSoundMax = readBytes(soundArchiveFile, 2);
	soundArchivePlayerInfo->streamTrackMax = readBytes(soundArchiveFile, 2);
	soundArchivePlayerInfo->streamChannelMax = readBytes(soundArchiveFile, 2);
	soundArchivePlayerInfo->waveSoundMax = readBytes(soundArchiveFile, 2);
	soundArchivePlayerInfo->streamBufferTimes = readBytes(soundArchiveFile, 1);
	fseek(soundArchiveFile, 1, SEEK_CUR);
	soundArchivePlayerInfo->options = readBytes(soundArchiveFile, 4);

	return STATUS_OK;
}

Status
readCTRSoundArchiveInfoPartition(struct CTRSoundArchiveInfoPartition *infoPartition, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList)
{
	infoPartition->header.filePosition = ftell(soundArchiveFile);
	CATCH(readPartitionHeader(&infoPartition->header.partitionHeader, soundArchiveFile, "INFO", readBytes) != STATUS_OK, "header", "info partition")

	infoPartition->body.filePosition = ftell(soundArchiveFile);
	for (u8 i = 0; i < 8; i += 1) {
		CATCH(readLink(&infoPartition->body.tableLinks[i], soundArchiveFile, readBytes) != STATUS_OK, "link", "info table")
	}

	/* Sound info */
	INFO(0, soundInfoLinkTable, soundInfo, struct CTRSoundInfo, readCTRSoundInfo)

	/* Sound group info */
	INFO(1, soundGroupInfoLinkTable, soundGroupInfo, struct CTRSoundGroupInfo, readCTRSoundGroupInfo)

	/* Bank info */
	INFO(2, bankInfoLinkTable, bankInfo, struct CTRBankInfo, readCTRBankInfo)

	/* Wave archive info */
	INFO(3, waveArchiveInfoLinkTable, waveArchiveInfo, struct CTRWaveArchiveInfo, readCTRWaveArchiveInfo)

	/* Group info */
	INFO(4, groupInfoLinkTable, groupInfo, struct CTRGroupInfo, readCTRGroupInfo)

	/* Player info */
	INFO(5, playerInfoLinkTable, playerInfo, struct CTRPlayerInfo, readCTRPlayerInfo)

	/* File info */
	INFO(6, fileInfoLinkTable, fileInfo, struct CTRFileInfo, readCTRFileInfo)

	/* Sound Archive Player Info */
	fseek(soundArchiveFile, infoPartition->body.filePosition + infoPartition->body.tableLinks[7].offset, SEEK_SET);
	CATCH(readCTRSoundArchivePlayerInfo(&infoPartition->body.soundArchivePlayerInfo, soundArchiveFile, readBytes) != STATUS_OK, "sound archive player info", "info partition")

	return STATUS_OK;
}

Status
readCTRSoundArchiveFilePartition(struct CTRSoundArchiveFilePartition *filePartition, FILE *soundArchiveFile, struct CTRSoundArchiveInfoPartition *infoPartition, u32 (*readBytes)(FILE *, u32), struct PointerList *pointerList)
{
	filePartition->header.filePosition = ftell(soundArchiveFile);
	CATCH(readPartitionHeader(&filePartition->header.partitionHeader, soundArchiveFile, "FILE", readBytes) != STATUS_OK, "header", "file partition")

	filePartition->body.filePosition = ftell(soundArchiveFile);

	ALLOCATE(filePartition->files, sizeof(char *) * infoPartition->body.fileInfoLinkTable.count)
	for (u32 i = 0; i < infoPartition->body.fileInfoLinkTable.count; i += 1) {
		if (infoPartition->body.fileInfo[i].toFileLocationInfo.referenceID == REFID_SOUNDARCHIVEFILE_INTERNALFILEINFO) {
			ALLOCATE(filePartition->files[i], infoPartition->body.fileInfo[i].internalFileInfo.toDataFromFilePartitionBody.length)
			fseek(soundArchiveFile, filePartition->body.filePosition + infoPartition->body.fileInfo[i].internalFileInfo.toDataFromFilePartitionBody.offset, SEEK_SET);
			fread(filePartition->files[i], 1, infoPartition->body.fileInfo[i].internalFileInfo.toDataFromFilePartitionBody.length, soundArchiveFile);
		}
	}

	return STATUS_OK;
}

Status
readCTRSoundArchive(CTRSoundArchive *soundArchive, FILE *soundArchiveFile)
{
	printf("Reading sound archive...\n");
	soundArchive->filePosition = ftell(soundArchiveFile);

	u32 (*readBytes)(FILE *file, u32 bytes);

	/* Set pointer list count */
	soundArchive->pointerList.count = 0;

	/* Header */
	printf("Reading sound archive file header...\n");
	CATCH(readCTRSoundArchiveHeader(&soundArchive->header, soundArchiveFile, &readBytes) != STATUS_OK, "header", "sound archive file")
	printf("File header read.\n");

	/* String Partition */
	printf("Reading sound archive string partition...\n");
	fseek(soundArchiveFile, soundArchive->header.partitionLinks[0].offset, SEEK_SET);
	CATCH(readCTRSoundArchiveStringPartition(&soundArchive->stringPartition, soundArchiveFile, readBytes, &soundArchive->pointerList) != STATUS_OK, "string partition", "sound archive file")
	printf("String partition read.\n");

	/* Info Partition */
	printf("Reading sound info partition...\n");
	fseek(soundArchiveFile, soundArchive->header.partitionLinks[1].offset, SEEK_SET);
	CATCH(readCTRSoundArchiveInfoPartition(&soundArchive->infoPartition, soundArchiveFile, readBytes, &soundArchive->pointerList) != STATUS_OK, "info partition", "sound archive file")
	printf("Info partition read.\n");

	/* File Partition */
	printf("Reading sound archive file partition...\n");
	fseek(soundArchiveFile, soundArchive->header.partitionLinks[2].offset, SEEK_SET);
	CATCH(readCTRSoundArchiveFilePartition(&soundArchive->filePartition, soundArchiveFile, &soundArchive->infoPartition, readBytes, &soundArchive->pointerList) != STATUS_OK, "file partition", "sound archive file")
	printf("File partition read.\n");

	printf("Sound archive read!\n");
	return STATUS_OK;
}