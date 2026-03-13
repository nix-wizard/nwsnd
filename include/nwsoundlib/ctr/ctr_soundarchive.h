/* See LICENSE file for copyright and license details. */
#include "../common.h"

struct CTRItemID {
	u32 filePosition;
	u32 ID; /* 3 bytes */
	u8 type;
};

struct CTRItemIDTable {
	u32 filePosition;
	u32 count;
	struct CTRItemID *table;
};

struct CTRSendValue {
	u32 filePosition;
	u8 mainSend;
	u8 fxSend[2];
	/* 1 byte padding */
};

typedef struct {
	struct PointerList pointerList;

	u32 filePosition;

	struct CTRSoundArchiveHeader{
		u32 filePosition;
		struct FileHeader fileHeader;
		struct LinkWithLength *partitionLinks;
		u32 stringOffset;
		u32 stringLength;
		u32 infoOffset;
		u32 infoLength;
		u32 fileOffset;
		u32 fileLength;
	} header;

	struct CTRSoundArchiveStringPartition {
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
			struct CTRPatriciaTree {
				u32 filePosition;
				u32 rootIndex;
				struct CTRNodeTable {
					u32 filePosition;
					u32 count;
					struct CTRNode {
						u32 filePosition;
						u16 flags;
						u16 bitIndex;
						u32 leftIndex;
						u32 rightIndex;
						u32 stringID;
						struct CTRItemID itemID;
					} *nodes;
					
					struct CTRNode **itemIDToNode;
				} nodeTable;
			} patriciaTree;
		} body;
	} stringPartition;

	struct CTRSoundArchiveInfoPartition {
		struct {
			u32 filePosition;
			struct PartitionHeader partitionHeader;
		} header;

		struct {
			u32 filePosition;
			struct Link tableLinks[8];

			struct LinkTable soundInfoLinkTable;
			struct CTRSoundInfo {
				u32 filePosition;
				u32 fileID;
				struct CTRItemID playerID;
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
				struct CTRSound3DInfo {
					u32 filePosition;
					u32 flags;
					f32 decayRatio;
					u8 delayCurve;
					u8 dopplerFactor;
					/* 2 byte padding */
					struct OptionParameter optionParameter;
				} sound3DInfo;

				/* Extra info types */
				struct CTRStreamSoundInfo {
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
					struct CTRStreamTrackInfo {
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
						struct CTRSendValue sendValue;
					} *streamTrackInfo;

					/* To send value */
					struct CTRSendValue sendValue;

					/* To stream sound extension */
					struct CTRStreamSoundExtension {
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

				struct CTRWaveSoundInfo {
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

				struct CTRSequenceSoundInfo {
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
			struct CTRSoundGroupInfo {
				u32 filePosition;
				struct CTRItemID startID; /* 3 bytes */
				struct CTRItemID endID; /* 3 bytes */
				struct Link toFileIdTable;
				struct Link toWaveSoundGroupInfo; /* 0x2205 */
				struct OptionParameter optionParameter;

				/* Parameters */
				u32 stringID; /* 0x00 */

				/* To file ID table */
				struct U32Table fileIDTable;

				/* To wave sound group info */
				struct CTRWaveSoundGroupInfo {
					u32 filePosition;
					struct Link toWaveArchiveItemIDTable;
					struct OptionParameter optionParameter;

					/* To table */
					struct CTRItemIDTable waveArchiveIDTable;
				} waveSoundGroupInfo;
			} *soundGroupInfo;

			struct LinkTable bankInfoLinkTable;
			struct CTRBankInfo {
				u32 filePosition;
				u32 fileID;
				struct Link toWaveArchiveItemIDTable;
				struct OptionParameter optionParameter;

				/* Parameters */
				u32 stringID; /* 0x00 */

				/* To table */
				struct CTRItemIDTable waveArchiveItemIDTable;
			} *bankInfo;

			struct LinkTable waveArchiveInfoLinkTable;
			struct CTRWaveArchiveInfo {
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
			struct CTRGroupInfo {
				u32 filePosition;
				u32 fileID;
				struct OptionParameter optionParameter;

				/* Parameters */
				u32 stringID; /* 0x00 */
			} *groupInfo;

			struct LinkTable playerInfoLinkTable;
			struct CTRPlayerInfo {
				u32 filePosition;
				u32 playableSoundMax;
				struct OptionParameter optionParameter;

				/* Parameters */
				u32 stringID; /* 0x00 */
				u32 heapSize;
			} *playerInfo;

			struct LinkTable fileInfoLinkTable;
			struct CTRFileInfo {
				u32 filePosition;
				struct Link toFileLocationInfo;
				struct OptionParameter optionParameter;

				/* File Info types */
				struct CTRInternalFileLocationInfo { /* 0x220c */
					u32 filePosition;
					struct LinkWithLength toDataFromFilePartitionBody; /* Offset or size not 0xffffffff */
				} internalFileInfo;

				struct CTRExternalFileLocationInfo { /* 0x220d */
					u32 filePosition;
					char *filePath; /* Null-terminated */
				} externalFileInfo;
			} *fileInfo;

			struct CTRSoundArchivePlayerInfo {
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

	struct CTRSoundArchiveFilePartition {
		struct {
			u32 filePosition;
			struct PartitionHeader partitionHeader;
		} header;
		
		struct {
			u32 filePosition;	
		} body;

		char **files;
	} filePartition;

} CTRSoundArchive;

Status
readCTRItemID(struct CTRItemID *itemID, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTRItemIDTable(struct CTRItemIDTable *itemIDTable, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRSendValue(struct CTRSendValue *sendValue, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTRSoundArchiveHeader(struct CTRSoundArchiveHeader *header, FILE *soundArchiveFile, u32 (**readBytesPointer)(FILE *, u32), struct PointerList *pointerList);

Status
readCTRNode(struct CTRNode *node, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes)) ;

Status
readCTRNodeTable(struct CTRNodeTable *nodeTable, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRPatriciaTree(struct CTRPatriciaTree *patriciaTree, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRSoundArchiveStringPartition(struct CTRSoundArchiveStringPartition *stringPartition, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRSound3DInfo(struct CTRSound3DInfo *sound3DInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTRStreamTrackInfo(struct CTRStreamTrackInfo *streamTrackInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRStreamSoundExtension(struct CTRStreamSoundExtension *streamSoundExtension, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTRStreamSoundInfo(struct CTRStreamSoundInfo *streamSoundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRWaveSoundInfo(struct CTRWaveSoundInfo *waveSoundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTRSequenceSoundInfo(struct CTRSequenceSoundInfo *sequenceSoundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRSoundInfo(struct CTRSoundInfo *soundInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRWaveSoundGroupInfo(struct CTRWaveSoundGroupInfo *waveSoundGroupInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRSoundGroupInfo(struct CTRSoundGroupInfo *soundGroupInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRBankInfo(struct CTRBankInfo *bankInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRWaveArchiveInfo(struct CTRWaveArchiveInfo *waveArchiveInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRGroupInfo(struct CTRGroupInfo *groupInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRPlayerInfo(struct CTRPlayerInfo *playerInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRInternalFileLocationInfo(struct CTRInternalFileLocationInfo *internalFileInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTRExternalFileLocationInfo(struct CTRExternalFileLocationInfo *externalFileInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTRFileInfo(struct CTRFileInfo *fileInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRSoundArchivePlayerInfo(struct CTRSoundArchivePlayerInfo *soundArchivePlayerInfo, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes));

Status
readCTRSoundArchiveInfoPartition(struct CTRSoundArchiveInfoPartition *infoPartition, FILE *soundArchiveFile, u32 (*readBytes)(FILE *file, u32 bytes), struct PointerList *pointerList);

Status
readCTRSoundArchiveFilePartition(struct CTRSoundArchiveFilePartition *filePartition, FILE *soundArchiveFile, struct CTRSoundArchiveInfoPartition *infoPartition, u32 (*readBytes)(FILE *, u32), struct PointerList *pointerList);

Status
readCTRSoundArchive(CTRSoundArchive *soundArchive, FILE *soundArchiveFile);