#include "fileManager.h"

int main(int argc, char *argv[])
{
	PARA_INFO para = {COMPRESS, false, ""};
	for (int i = 0; i < argc; i++)
	{
		printf("[%d] %s\n", i, argv[i]);
		switch (i)
		{
		case PARA_MODE:
			para.mode = (COMPRESS_MODE)std::atoi(argv[i]);
			break;
		case PARA_FILTER:
			para.filter = std::atoi(argv[i]) != 0;
			break;
		case PARA_PATH:
			para.songPath = argv[i];
			break;
		}
	}

	switch (para.mode)
	{
	case COMPRESS:
		CompressSongDir(para);
		break;
	case DECOMPRESS:
		DecompressSongFolder();
		break;
	default:
		break;
	}
	return 0;
}