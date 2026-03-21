/* See LICENSE file for copyright and license details. */
#ifndef NWSND_CTR_SOUNDARCHIVE_H
#define NWSND_CTR_SOUNDARCHIVE_H

#include "nwsnd/common.h"

struct CTR_SoundArchive_ItemID {
	u32 filePosition;
	u32 ID; /* 3 bytes */
	u8 type;
};

struct CTR_SoundArchive_ItemIDTable {
	u32 filePosition;
	u32 count;
	struct CTR_SoundArchive_ItemID *table;
};

struct CTR_SoundArchive_SendValue {
	u32 filePosition;
	u8 mainSend;
	u8 fxSend[2];
	/* 1 byte padding */
};

typedef struct {
	struct PointerList pointerList;

	u32 filePosition;

	struct CTR_SoundArchive_FileHeader{
		u32 filePosition;
		struct FileHeader fileHeader;
		struct LinkWithLength partitionLinks[3];
	} header;

	struct CTR_SoundArchive_StringPartition {
		struct {
			u32 filePosition;
			struct PartitionHeader partitionHeader;
		} header;
		struct {
			u32 filePosition;
			struct Link tableLinks[2];
			u32 filenameLinksOffset;
			u32 patriciaTreeOffset;
			struct LinkWithLengthTable filenameLinkTable;
			struct {
				u32 filePosition;
				char *filename;
			} *filenameTable;
			struct CTR_SoundArchive_PatriciaTree {
				u32 filePosition;
				u32 rootIndex;
				struct CTR_SoundArchive_NodeTable {
					u32 filePosition;
					u32 count;
					struct CTR_SoundArchive_Node {
						u32 filePosition;
						u16 flags;
						u16 bitIndex;
						u32 leftIndex;
						u32 rightIndex;
						u32 stringID;
						struct CTR_SoundArchive_ItemID itemID;
					} *nodes;
					
					struct CTR_SoundArchive_Node **itemIDToNode;
				} nodeTable;
			} patriciaTree;
		} body;
	} stringPartition;

	struct CTR_SoundArchive_InfoPartition {
		struct {
			u32 filePosition;
			struct PartitionHeader partitionHeader;
		} header;

		struct {
			u32 filePosition;
			struct Link tableLinks[8];

			struct LinkTable soundInfoLinkTable;
			struct CTR_SoundArchive_SoundInfo {
				u32 filePosition;
				u32 fileID;
				struct CTR_SoundArchive_ItemID playerID;
				u8 volume;
				/* 3 byte padding */
				struct Link extraInfoLink;
				struct OptionParameter optionParameter;
				/* Params */
				u32 stringID; /* 0x00 */
				u32 panParam;
				u32 playerParam;
				u32 offsetTo3DParam; /* 0x8 */
				u32 offsetToSendParam;
				u32 offsetToModParam;
				u32 offsetToRVLParam; /* 0x10 */
				u32 offsetToCTRParam;
				u32 userParam3; /* 0x1c */
				u32 userParam2;
				u32 userParam1;
				u32 userParam0;

				/* Pan Param */
				struct {
					u8 panMode; /* 0 */
					u8 panCurve; /* 1 */
				} panParams;

				/* Player Param */
				struct {
					u8 playerPriority; /* 0 */
					u8 playerID; /* 1 */
				} playerParams;

				Bool isFrontBypass;

				/* To 3D sound info */
				struct CTR_SoundArchive_Sound3DInfo {
					u32 filePosition;
					u32 flags;
					f32 decayRatio;
					u8 delayCurve;
					u8 dopplerFactor;
					/* 2 byte padding */
					struct OptionParameter optionParameter;
				} sound3DInfo;

				/* Extra info types */
				struct CTR_SoundArchive_StreamSoundInfo {
					u32 filePosition;
					u16 allocateTrackFlags;
					u16 allocateChannelCount;
					struct Link toStreamTrackInfoLinkTable; /* 0x0101 */
					f32 pitch;
					struct Link toSendValue;
					struct Link toStreamSoundExtension; /* 0x2210 & offset not 0xffffffff */
					u32 prefetchFileID;

					/* To stream track info link table */
					struct LinkTable streamTrackInfoLinkTable;

					/* To stream track info */
					struct CTR_SoundArchive_StreamTrackInfo {
						u32 filePosition;
						u8 volume;
						u8 pan;
						u8 span;
						u8 flags;
						struct Link toGlobalChannelIndexTable;
						struct Link toSendValue; /* 0x220F */
						u8 lpfFreq;
						u8 biquadType;
						u8 biquadValue;
						/* 1 byte padding */

						/* To global channel index table */
						struct U8Table globalChannelIndexTable;

						/* To send value */
						struct CTR_SoundArchive_SendValue sendValue;
					} *streamTrackInfo;

					/* To send value */
					struct CTR_SoundArchive_SendValue sendValue;

					/* To stream sound extension */
					struct CTR_SoundArchive_StreamSoundExtension {
						u32 filePosition;
						u32 streamTypeInfo;
						u32 loopStartFrame;
						u32 loopEndFrame;

						struct {
							u8 streamType; /* 0 */
							Bool loopFlag; /* 1 */
						} streamTypeInfoParams;
					} streamSoundExtension;
				} streamSoundInfo;
				/* TODO: legacy stream sound info */

				struct CTR_SoundArchive_WaveSoundInfo {
					u32 filePosition;
					u32 index;
					u32 allocateTrackCount;
					struct OptionParameter optionParameter;
					/* Parameters */
					u32 priority; /* 0x00 */

					struct {
						u8 priorityChannelPriority; /* 0 */
						Bool isReleasePriorityFix; /* 1 */
					} priorityParams;
				} waveSoundInfo;

				struct CTR_SoundArchive_SequenceSoundInfo {
					u32 filePosition;
					struct Link toBankIDTable;
					u32 allocateTrackFlags;
					struct OptionParameter optionParameter;
					/* Parameters */
					u32 startOffset; /* 0x00 */
					u32 priority;

					struct {
						u8 priorityChannelPriority; /* 0 */
						Bool isReleasePriorityFix; /* 1 */
					} priorityParams;

					/* To bank ID table */
					struct U32Table bankIDTable;
				} sequenceSoundInfo;
			} *soundInfo;

			struct LinkTable soundGroupInfoLinkTable;
			struct CTR_SoundArchive_SoundGroupInfo {
				u32 filePosition;
				struct CTR_SoundArchive_ItemID startID; /* 3 bytes */
				struct CTR_SoundArchive_ItemID endID; /* 3 bytes */
				struct Link toFileIdTable;
				struct Link toWaveSoundGroupInfo; /* 0x2205 */
				struct OptionParameter optionParameter;

				/* Parameters */
				u32 stringID; /* 0x00 */

				/* To file ID table */
				struct U32Table fileIDTable;

				/* To wave sound group info */
				struct CTR_SoundArchive_WaveSoundGroupInfo {
					u32 filePosition;
					struct Link toWaveArchiveItemIDTable;
					struct OptionParameter optionParameter;

					/* To table */
					struct CTR_SoundArchive_ItemIDTable waveArchiveIDTable;
				} waveSoundGroupInfo;
			} *soundGroupInfo;

			struct LinkTable bankInfoLinkTable;
			struct CTR_SoundArchive_BankInfo {
				u32 filePosition;
				u32 fileID;
				struct Link toWaveArchiveItemIDTable;
				struct OptionParameter optionParameter;

				/* Parameters */
				u32 stringID; /* 0x00 */

				/* To table */
				struct CTR_SoundArchive_ItemIDTable waveArchiveItemIDTable;
			} *bankInfo;

			struct LinkTable waveArchiveInfoLinkTable;
			struct CTR_SoundArchive_WaveArchiveInfo {
				u32 filePosition;
				u32 fileID;
				Bool isLoadIndividual;
				/* 3 byte padding */
				struct OptionParameter optionParameter;

				/* Parameters */
				u32 stringID; /* 0x00 */
				u32 waveCount;
			} *waveArchiveInfo;

			struct LinkTable groupInfoLinkTable;
			struct CTR_SoundArchive_GroupInfo {
				u32 filePosition;
				u32 fileID;
				struct OptionParameter optionParameter;

				/* Parameters */
				u32 stringID; /* 0x00 */
			} *groupInfo;

			struct LinkTable playerInfoLinkTable;
			struct CTR_SoundArchive_PlayerInfo {
				u32 filePosition;
				u32 playableSoundMax;
				struct OptionParameter optionParameter;

				/* Parameters */
				u32 stringID; /* 0x00 */
				u32 heapSize;
			} *playerInfo;

			struct LinkTable fileInfoLinkTable;
			struct CTR_SoundArchive_FileInfo {
				u32 filePosition;
				struct Link toFileLocationInfo;
				struct OptionParameter optionParameter;

				/* File Info types */
				struct CTR_SoundArchive_InternalFileLocationInfo { /* 0x220c */
					u32 filePosition;
					struct LinkWithLength toDataFromFilePartitionBody; /* Offset or size not 0xffffffff */
				} internalFileInfo;

				struct CTR_SoundArchive_ExternalFileLocationInfo { /* 0x220d */
					u32 filePosition;
					char *filePath; /* Null-terminated */
				} externalFileInfo;
			} *fileInfo;

			struct CTR_SoundArchive_SoundArchivePlayerInfo {
				u32 filePosition;
				u16 sequenceSoundMax;
				u16 sequenceTrackMax;
				u16 streamSoundMax;
				u16 streamTrackMax;
				u16 streamChannelMax;
				u16 waveSoundMax;
				u16 waveTrackMax;
				u8 streamBufferTimes;
				/* 1 byte padding */
				u32 options;
			} soundArchivePlayerInfo;
		} body;
	} infoPartition;

	struct CTR_SoundArchive_FilePartition {
		struct {
			u32 filePosition;
			struct PartitionHeader partitionHeader;
		} header;
		
		struct {
			u32 filePosition;	
		} body;

		char **files;
	} filePartition;

} CTR_SoundArchive;

Status
readCTR_SoundArchive_ItemID(struct CTR_SoundArchive_ItemID *itemID, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTR_SoundArchive_ItemIDTable(struct CTR_SoundArchive_ItemIDTable *itemIDTable, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_SendValue(struct CTR_SoundArchive_SendValue *sendValue, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTR_SoundArchive_FileHeader(struct CTR_SoundArchive_FileHeader *header, FILE *soundArchiveFile, u32 (**readBytesPointer)(FILE *, u32));

Status
readCTR_SoundArchive_Node(struct CTR_SoundArchive_Node *node, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes)) ;

Status
readCTR_SoundArchive_NodeTable(struct CTR_SoundArchive_NodeTable *nodeTable, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_PatriciaTree(struct CTR_SoundArchive_PatriciaTree *patriciaTree, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_StringPartition(struct CTR_SoundArchive_StringPartition *stringPartition, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_Sound3DInfo(struct CTR_SoundArchive_Sound3DInfo *sound3DInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTR_SoundArchive_StreamTrackInfo(struct CTR_SoundArchive_StreamTrackInfo *streamTrackInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_StreamSoundExtension(struct CTR_SoundArchive_StreamSoundExtension *streamSoundExtension, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTR_SoundArchive_StreamSoundInfo(struct CTR_SoundArchive_StreamSoundInfo *streamSoundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_WaveSoundInfo(struct CTR_SoundArchive_WaveSoundInfo *waveSoundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTR_SoundArchive_SequenceSoundInfo(struct CTR_SoundArchive_SequenceSoundInfo *sequenceSoundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_SoundInfo(struct CTR_SoundArchive_SoundInfo *soundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_WaveSoundGroupInfo(struct CTR_SoundArchive_WaveSoundGroupInfo *waveSoundGroupInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_SoundGroupInfo(struct CTR_SoundArchive_SoundGroupInfo *soundGroupInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_BankInfo(struct CTR_SoundArchive_BankInfo *bankInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_WaveArchiveInfo(struct CTR_SoundArchive_WaveArchiveInfo *waveArchiveInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_GroupInfo(struct CTR_SoundArchive_GroupInfo *groupInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_PlayerInfo(struct CTR_SoundArchive_PlayerInfo *playerInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_InternalFileLocationInfo(struct CTR_SoundArchive_InternalFileLocationInfo *internalFileInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTR_SoundArchive_ExternalFileLocationInfo(struct CTR_SoundArchive_ExternalFileLocationInfo *externalFileInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTR_SoundArchive_FileInfo(struct CTR_SoundArchive_FileInfo *fileInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_SoundArchivePlayerInfo(struct CTR_SoundArchive_SoundArchivePlayerInfo *soundArchivePlayerInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTR_SoundArchive_InfoPartition(struct CTR_SoundArchive_InfoPartition *infoPartition, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTR_SoundArchive_FilePartition(struct CTR_SoundArchive_FilePartition *filePartition, FILE *soundArchiveFile, struct CTR_SoundArchive_InfoPartition *infoPartition, u32 (*readBytes)(FILE *, u32), struct PointerList *pointerList);

Status
readCTR_SoundArchive(CTR_SoundArchive *soundArchive, FILE *soundArchiveFile);

#endif