/* See LICENSE file for copyright and license details. */
#include "nwsnd/ctr/ctr_soundarchive.h"

int
main(int argc, char **argv)
{
	FILE *soundArchiveFile;
	if (argc == 2) {
		soundArchiveFile = fopen(argv[1], "rb");
		if (!soundArchiveFile) {
			perror("fopen");
			return 1;
		}
		
		CTRSoundArchive soundArchive;
		if (readCTRSoundArchive(&soundArchive, soundArchiveFile) != STATUS_OK) {
			fprintf(stderr, "Failed to read sound archive!\n");
			return 1;
		}
		
		fclose(soundArchiveFile);
		freePointerList(soundArchive.pointerList);
	}
	else {
		printf("usage: %s file.bcsar\n", argv[0]);
		return 1;
	}

	return 0;
}