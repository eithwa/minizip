#include <iostream>
#include <filesystem>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/nowide/fstream.hpp>

namespace fs = boost::filesystem;
enum COMPRESS_MODE
{
	COMPRESS,
	DECOMPRESS
};

enum PARA_MODE
{
	PARA_NAME,
	PARA_MODE,
	PARA_FILTER,
	PARA_PATH,
	PARA_NUM
};

struct PARA_INFO
{
	COMPRESS_MODE mode;
	bool filter;
	boost::filesystem::wpath songPath;
	boost::filesystem::wpath tempPath;
};

bool CompressSongDir(PARA_INFO& para);
void DecompressSongFolder(void);