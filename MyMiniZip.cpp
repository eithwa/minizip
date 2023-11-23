#include "MyMiniZip.h"
#include <fstream>
#include <algorithm>
#include <dbghelp.h>
#include <shlwapi.h>
#include <iostream>
#include <sstream>
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"dbghelp.lib")
#if _DEBUG 
#pragma comment(lib,"zlibstatD.lib")
#else
#pragma comment(lib,"zlibstat.lib")
#endif
MyCTime::MyCTime()
{
}

MyCTime::~MyCTime()
{
}

Time MyCTime::start() const
{
	std::chrono::system_clock::time_point systemTimePoint = std::chrono::system_clock::now();

	auto durationSinceEpoch = systemTimePoint.time_since_epoch();
	auto durationInSteadyClock = std::chrono::duration_cast<std::chrono::steady_clock::duration>(durationSinceEpoch);

	Time steadyTimePoint = Time(durationInSteadyClock);
	return steadyTimePoint;
}

Time MyCTime::end() const
{
	std::chrono::system_clock::time_point systemTimePoint = std::chrono::system_clock::now();

	auto durationSinceEpoch = systemTimePoint.time_since_epoch();
	auto durationInSteadyClock = std::chrono::duration_cast<std::chrono::steady_clock::duration>(durationSinceEpoch);


	Time steadyTimePoint = Time(durationInSteadyClock);
	return steadyTimePoint;
}

int MyCTime::CountInterval(Time& begin, Time& end)
{
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
	return  double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
}

MyMiniZip::MyMiniZip()
:file_status(""),
global_comment(""),
unZipFileHandle(NULL),
nCountTime(0)
{
}


MyMiniZip::~MyMiniZip()
{
}

int MyMiniZip::GetCountTime() const
{
	return nCountTime;
}

std::string MyMiniZip::GetFileStatus() const
{
	return file_status;
}

std::string MyMiniZip::GetGlobalComment() const
{
	return global_comment;
}

bool MyMiniZip::addFileZip(zipFile ZipFile, const std::string strFileInZipName, const std::string strPath)
{
	//初始化写入zip的文件信息  
	bool bResult = false;
	zip_fileinfo zi;
	ZeroMemory(&zi, sizeof(zip_fileinfo));
	std::string strFIlename = strFileInZipName;
	std::string strReadFile;
	if (strPath.empty())
		strFIlename += "/";
	//在zip文件中创建新文件  
	zipOpenNewFileInZip(ZipFile, strFIlename.c_str(), &zi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
	if (!strPath.empty())
	{
		//strReadFile = strPath + strFileInZipName;
		LPVOID pBuffer = nullptr;
		DWORD dwLength = ReadFileBuffer(strPath, &pBuffer);
		if (dwLength && pBuffer)
		{
			zipWriteInFileInZip(ZipFile, pBuffer, dwLength);
			delete pBuffer;
			pBuffer = nullptr;
			bResult = true;
		}
	}
	zipCloseFileInZip(ZipFile);
	return bResult;
}

bool MyMiniZip::packFolderToZip(zipFile ZipFile, const std::string strFileInZipName, const std::string strPath)
{
	WIN32_FIND_DATAA file_dta;
	std::string filePath = strFileInZipName;
	CHAR szFullFilePath[MAX_PATH];
	CHAR szTmpFolderPath[MAX_PATH];
	CHAR szTmpFileName[MAX_PATH];
	DWORD dwEerror = NULL;
	CHAR* findStr = nullptr;
	int lens = NULL;
	ZeroMemory(&file_dta, sizeof(WIN32_FIND_DATAA));
	ZeroMemory(szFullFilePath, MAX_PATH);
	ZeroMemory(szTmpFolderPath, MAX_PATH);
	ZeroMemory(szTmpFileName, MAX_PATH);
	lstrcpyA(szFullFilePath, filePath.c_str());
	findStr = StrRStrIA(filePath.c_str(), NULL, "\\");
	if (!findStr)
		return dwEerror;
	lens = lstrlenA(findStr);
	if (lens > 1)
	{
		lstrcatA(szFullFilePath, "\\*.*");
	}
	else if (lens == 1)
	{
		lstrcatA(szFullFilePath, "*.*");
	}
	else
	{
		return dwEerror;
	}
	HANDLE hFile = INVALID_HANDLE_VALUE;
	hFile = FindFirstFileA(szFullFilePath, &file_dta);
	dwEerror = ::GetLastError();
	if (INVALID_HANDLE_VALUE == hFile)
		return dwEerror;
	while (::FindNextFileA(hFile, &file_dta))
	{
		if (!lstrcmpiA(file_dta.cFileName, ".") || !lstrcmpiA(file_dta.cFileName, ".."))
			continue;
		sprintf_s(szFullFilePath, "%s\\%s", filePath.c_str(), file_dta.cFileName);
		if (file_dta.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			ZeroMemory(szTmpFolderPath, MAX_PATH);
			if (strPath.empty())
			{
				sprintf_s(szTmpFolderPath, "%s", file_dta.cFileName);
			}
			else
			{
				sprintf_s(szTmpFolderPath, "%s/%s", strPath.c_str(), file_dta.cFileName);
			}
			addFileZip(ZipFile, szTmpFolderPath, "");
			packFolderToZip(ZipFile, szFullFilePath, szTmpFolderPath);
			continue;
		}
		else
		{
			if (strPath.empty())
			{
				addFileZip(ZipFile, file_dta.cFileName, szFullFilePath);
			}
			else
			{
				ZeroMemory(szTmpFileName, MAX_PATH);
				if (strPath.empty())
				{
					addFileZip(ZipFile, file_dta.cFileName, szFullFilePath);
				}
				else
				{
					sprintf_s(szTmpFileName, "%s/%s", strPath.c_str(), file_dta.cFileName);
					addFileZip(ZipFile, szTmpFileName, szFullFilePath);
					std::cout << "Zip Compressed File\t" << szFullFilePath << std::endl;
				}
			}
		}
	}
	::FindClose(hFile);
	return dwEerror;
}

bool MyMiniZip::CompressToPackageZip(const std::string strSourceFile, const std::string strSaveFile)
{
	MyCTime TimeCount;
	TimeBegin = TimeCount.start();
	bool bResult = false;
	DWORD dwAttrib = INVALID_FILE_ATTRIBUTES;
	if (strSourceFile.empty() || strSaveFile.empty()) return bResult;
	if (!PathFileExistsA(strSourceFile.c_str()) || !PathFileExistsA(strSourceFile.c_str())) return bResult;
	dwAttrib = ::GetFileAttributesA(strSourceFile.c_str());
	if (dwAttrib == INVALID_FILE_ATTRIBUTES) return bResult;
	if (NULL != (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		//表明是一个文件夹
		bResult = CompressToPackageZip(strSourceFile, strSaveFile, 1);
	}
	else
	{
		//表明是一个文件
		bResult = CompressToPackageZip(strSourceFile, strSaveFile, 0);
	}
	TimeEnd = TimeCount.end();
	nCountTime = TimeCount.CountInterval(TimeBegin, TimeEnd);
	return bResult;
}

bool MyMiniZip::CompressToPackageZip(const std::string strSourceFile, const std::string strSaveFile, int nMode)
{
	bool bResult = false;
	zipFile zipFileHandle = NULL;
	//Create a zip archive file. Returns null on failure.
	zipFileHandle = zipOpen(strSaveFile.c_str(), APPEND_STATUS_CREATE);
	if (!zipFileHandle) return bResult;
	if (nMode)//folder
	{
		std::string filePath = strSourceFile;
		size_t lastSlash = filePath.find_last_of('\\');
		std::string lastFolderName = filePath.substr(lastSlash + 1);
		bResult = packFolderToZip(zipFileHandle, strSourceFile, lastFolderName);
	}
	else
	{
		CHAR szFileName[MAX_PATH];
		ZeroMemory(szFileName, MAX_PATH);
		lstrcpyA(szFileName, strSourceFile.c_str());
		PathStripPathA(szFileName);
		bResult = addFileZip(zipFileHandle, szFileName, strSourceFile);
	}
	zipClose(zipFileHandle,nullptr);
	return bResult;
}

bool DoesFolderExist(const std::string &folderPath)
{
	DWORD attributes = GetFileAttributesA(folderPath.c_str());
	return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY));
}

std::string GenerateNewPath(const std::string &oldPath)
{
	if (!DoesFolderExist(oldPath))
	{
		return "";
	}

	std::string newPath = oldPath;
	int counter = 1;

	while (DoesFolderExist(newPath))
	{
		size_t pos = oldPath.find_last_of("\\");
		if (pos != std::string::npos)
		{
			newPath = oldPath.substr(0, pos + 1) + oldPath.substr(pos + 1) + "(" + std::to_string(counter++) + ")";
		}
		else
		{
			newPath = oldPath + "(" + std::to_string(counter++) + ")";
		}
	}

	return newPath;
}

std::string GetLastFolderName(const std::string &path)
{
	size_t lastSlash = path.find_last_of('\\');

	if (lastSlash != std::string::npos)
	{
		return path.substr(lastSlash + 1);
	}

	return path;
}

std::string GetFirstFolderName(const std::string &path)
{
	std::string folderName;
	size_t firstSlash = path.find('\\');

	if (firstSlash != std::string::npos)
	{
		folderName = path.substr(0, firstSlash);
	}
	else
	{
		folderName = path;
	}

	return folderName;
}

std::string ChangeFirstPathComponent(const std::string &originalPath, const std::string &newComponent)
{
	std::istringstream iss(originalPath);
	std::string component;
	std::string result;

	if (std::getline(iss, component, '\\'))
	{
		result = newComponent + originalPath.substr(component.length());
	}
	else
	{
		result = newComponent;
	}

	return result;
}

std::string RenameUnZipFolder(std::string zipPath, std::string rootPath, std::string &newFolderName, bool &initFolderName)
{
	std::string newZipPath;
	replace(zipPath.begin(), zipPath.end(), '/', '\\');
	std::string folderName = GetFirstFolderName(zipPath);
	if(!initFolderName)
	{
		initFolderName = true;
		std::string oldPath = rootPath + folderName;
		replace(oldPath.begin(), oldPath.end(), '/', '\\');
		if (DoesFolderExist(oldPath))
		{
			// if old path is xxxxx(1) new path is xxxxx(2)
			// if old path is xxxxx new path is xxxx(1)
			newFolderName = GetLastFolderName(GenerateNewPath(oldPath));
		}
		else
		{
			newFolderName = folderName;
		}
		newZipPath = ChangeFirstPathComponent(zipPath, newFolderName);
	}
	else
	{
		newZipPath = ChangeFirstPathComponent(zipPath, newFolderName);
	}
	return newZipPath;
}

DWORD MyMiniZip::unZipPackageToLoacal(const std::string strSourceZipPath, const std::string strDestZipPath)
{
	MyCTime TimeCount;
	TimeBegin = TimeCount.start();
	DWORD dwResult = NULL;
	std::string rootPath;
	if (strSourceZipPath.empty() || strDestZipPath.empty()) return dwResult;
	if (!PathFileExistsA(strDestZipPath.c_str()) || !PathFileExistsA(strSourceZipPath.c_str())) return dwResult;
	unZipFileHandle = unzOpen(strSourceZipPath.c_str());
	if (!unZipFileHandle)return dwResult;
	rootPath = strDestZipPath;
	if (strDestZipPath.at(strDestZipPath.length() - 1) != '\\')
		rootPath += "\\";
	unz_global_info global_info;
	if (UNZ_OK != unzGetGlobalInfo(unZipFileHandle, &global_info))
	{
		file_status = "Failed to obtain global zip information!";
		unzClose(unZipFileHandle);
		unZipFileHandle = NULL;
		return dwResult;
	}
	/*获取zip注释内容*/
	global_comment = "";
	if (global_info.size_comment > 0)
	{
		PSTR pComment = new char[global_info.size_comment + 1];
		if (pComment)
		{
			ZeroMemory(pComment, global_info.size_comment + 1);
			int nResult = unzGetGlobalComment(unZipFileHandle, pComment, global_info.size_comment + 1);
			if (nResult > 0)
				global_comment = pComment;
			delete[] pComment;
			pComment = nullptr;
		}
	}
	int zipinfo = unzGoToFirstFile(unZipFileHandle);
	if (UNZ_OK != zipinfo)
	{
		file_status = "Unable to obtain file information in the zip package. The zip package may be null!";
		unzClose(unZipFileHandle);
		unZipFileHandle = NULL;
		return dwResult;
	}
	std::string newFolderName = "";
	bool initFolderName = false;
	while (UNZ_OK == zipinfo)
	{
		unz_file_info file_info;
		char szZipName[MAX_PATH];
		std::string strFileName;
		ZeroMemory(szZipName, MAX_PATH);
		unzGetCurrentFileInfo(unZipFileHandle, &file_info, szZipName, MAX_PATH, NULL, 0, NULL, 0);
		strFileName = szZipName;
		if (strFileName.empty())
			continue;
		int length = strFileName.length() - 1;
		strFileName = RenameUnZipFolder(strFileName, rootPath, newFolderName, initFolderName);
		if (strFileName[length] != '/')
		{
			std::string Filepath = rootPath + strFileName;
			replace(Filepath.begin(), Filepath.end(), '/', '\\');
			if (UNZ_OK == unzOpenCurrentFile(unZipFileHandle))
			{
				PVOID FilePtr = nullptr;
				FilePtr = new BYTE[file_info.uncompressed_size];
				if (!FilePtr)
					continue;
				ZeroMemory(FilePtr, file_info.uncompressed_size);
				int dwLens = unzReadCurrentFile(unZipFileHandle, FilePtr, file_info.uncompressed_size);
				if (dwLens == file_info.uncompressed_size)
				{
					dwLens = WriteFileBuffer(Filepath, FilePtr, dwLens);
				}
				delete[] FilePtr;
				FilePtr = nullptr;
				unzCloseCurrentFile(unZipFileHandle);
				printf_s("Zip Create File: \t %s \n", Filepath.c_str());
			}
		}
		else
		{
			std::string Filepath = rootPath + strFileName;
			replace(Filepath.begin(), Filepath.end(), '/', '\\');
			MakeSureDirectoryPathExists(Filepath.c_str());
			printf_s("Zip Create Folder: \t %s \n", Filepath.c_str());
		}
		zipinfo = unzGoToNextFile(unZipFileHandle);
	}
	dwResult = global_info.number_entry;
	unzClose(unZipFileHandle);
	unZipFileHandle = NULL;
	TimeEnd = TimeCount.end();
	nCountTime = TimeCount.CountInterval(TimeBegin, TimeEnd);
	return dwResult;
}

DWORD MyMiniZip::ReadFileBuffer(std::string szFileName, PVOID* pFileBuffer)
{
	DWORD dwFileSize = NULL;
	DWORD dwAttribut = NULL;
	std::ifstream file;
	if (szFileName.empty())return dwFileSize;
	dwAttribut = ::GetFileAttributesA(szFileName.c_str());
	if (dwAttribut == INVALID_FILE_ATTRIBUTES || 0 != (dwAttribut & FILE_ATTRIBUTE_DIRECTORY))
	{
		dwFileSize = INVALID_FILE_ATTRIBUTES;
		return dwFileSize;
	}
	file.open(szFileName, std::ios::in | std::ios::binary);
	if (!file.is_open())
		return dwFileSize;
	file.seekg(0l, std::ios::end);
	dwFileSize = file.tellg();
	file.seekg(0l, std::ios::beg);
	*pFileBuffer = new BYTE[dwFileSize];
	if (!*pFileBuffer)
	{
		dwFileSize = NULL;
		file.close();
		return dwFileSize;
	}
	ZeroMemory(*pFileBuffer, dwFileSize);
	file.read((char*)*pFileBuffer, dwFileSize);
	file.close();
	return dwFileSize;
}

DWORD MyMiniZip::WriteFileBuffer(std::string szFileNmae, PVOID pFileBuffer, DWORD dwFileSize)
{
	DWORD dwStates = -1;
	std::string szFilePath, strName;
	size_t npos = -1;
	if (szFileNmae.empty())
		return dwStates;
	npos = szFileNmae.rfind("\\");
	if (npos != std::string::npos)
	{
		szFilePath = szFileNmae.substr(0, npos + 1);
		if (!szFilePath.empty())
		{
			if (MakeSureDirectoryPathExists(szFilePath.c_str()))
			{
				HANDLE hFile = INVALID_HANDLE_VALUE;
				DWORD dwError = NULL;
				hFile = CreateFileA(szFileNmae.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
				dwError = ::GetLastError();
				if (hFile != INVALID_HANDLE_VALUE)
				{
					::WriteFile(hFile, pFileBuffer, dwFileSize, &dwStates, NULL);
					/*	CloseHandle(hFile);
					ofstream file(szFileNmae, std::ios::out|std::ios::binary);
					//file.open(szFileNmae,std::ios::out);
					if (!file.is_open())
					return dwStates;
					file.write((char*)pFileBuffer, dwFileSize);
					file.close();
					dwStates = NULL;*/
					::CloseHandle(hFile);
				}
			}
		}
	}

	return dwStates;
}
