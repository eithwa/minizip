#include <fstream>
#include <regex>
#include <codecvt>
#include <locale>
#include <unicode/utypes.h>
#include <unicode/unistr.h>
#include <unicode/translit.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include "fileManager.h"
#include "MyMiniZip.h"

namespace io = boost::iostreams;
namespace fs = boost::filesystem;

unsigned long GetFileSizeInKB(FILE* file)
{
	unsigned long currentPosition = ftell(file); // Save the current file pointer position
	unsigned long fileSize = 0;

	fseek(file, 0L, SEEK_END);				// Move the file pointer to the end of the file
	fileSize = ftell(file);					// Get file size
	fseek(file, currentPosition, SEEK_SET); // Restore file pointer position

	return fileSize / 1024; // Return file size (KB)
}

bool CopyFolder(PARA_INFO& para, const fs::wpath& destinationPath)
{
	bool result = false;
	try
	{
		// Check if the source path exists
		if (!fs::exists(para.songPath))
		{
			std::wcerr << L"Source path does not exist." << std::endl;
			result = false;
		}
		else
		{
			// Create destination directory if it doesn't exist
			if (!fs::exists(destinationPath))
			{
				fs::create_directories(destinationPath);
			}

			// Append the last level directory name to the destination path
			fs::wpath destinationWithSourceDir = destinationPath / para.songPath.filename();
			
			// Copy the folder using std::filesystem::copy
			fs::copy(para.songPath, destinationWithSourceDir, fs::copy_options::recursive);
			para.tempPath = destinationWithSourceDir;
			// std::wcout << L"Folder copied successfully." << std::endl;
			result = true;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error copying folder: " << e.what() << std::endl;
		result = false;
	}
	return result;
}

bool DeleteFolder(boost::filesystem::wpath &path)
{
	bool result = true;
	try
	{
		boost::filesystem::remove_all(path);

		// std::cout << "Folder deleted successfully." << std::endl;
		result = true;
	}
	catch (const boost::filesystem::filesystem_error &ex)
	{
		std::cerr << "Error deleting folder: " << ex.what() << std::endl;
		result = false;
	}
	return result;
}

bool PathExists(const std::string& path)
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

std::string Transliterate(const std::wstring &source)
{
	std::string result;
	// Convert std::wstring to UTF-16 encoded string
	std::u16string utf16String(source.begin(), source.end());

	// Create a UnicodeString from the UTF-16 encoded string
	icu::UnicodeString unicodeString(reinterpret_cast<const UChar *>(utf16String.c_str()), utf16String.length());

	// Create a Transliterator object
	UErrorCode status = U_ZERO_ERROR;
	icu::Transliterator *trans = icu::Transliterator::createInstance("Any-Latin; Latin-ASCII", UTRANS_FORWARD, status);

	if (U_SUCCESS(status))
	{
		// Translate
		trans->transliterate(unicodeString);

		// Output results
		unicodeString.toUTF8String(result);
		// Use UTF-8 string output
		result.erase(std::remove_if(result.begin(), result.end(), ::isspace), result.end());

		// Release resources
		delete trans;
	}
	else
	{
		std::cerr << "Transliterator creation failed with status: " << u_errorName(status) << std::endl;
	}
	return result;
}

// Function to check if a string contains only English letters, digits, or symbols
bool IsEnglishOrSymbol(const std::wstring &str)
{
	// Regular expression: ^[A-Za-z0-9 _\-\[\]\{\}\(\)]*$
	// ^ start
	// [A-Za-z0-9] matches uppercase letters, lowercase letters, and numbers
	// _\-[\]\{\}\(\) matches underscore, hyphen, square brackets, curly braces, and parentheses
	// * Zero or more matches
	// $ end
	std::wregex regex(L"^[A-Za-z0-9 _.\\-\\[\\]\\{\\}\\(\\)]*$");

	return std::regex_match(str, regex);
}

std::wstring FindRegex(const std::wstring &filePath, std::wregex regex)
{
	try
	{
		// Open the .sm file
		std::wifstream file(filePath);
		if (!file.is_open())
		{
			std::wcerr << L"Error opening file: " << filePath << std::endl;
			return L"";
		}

		std::wstring line;
		while (std::getline(file, line))
		{
			// Search for the specified regex pattern in each line
			std::wsmatch match;
			if (std::regex_search(line, match, regex))
			{
				if (match.size() > 1)
				{
					// Extract the matched information
					std::wstring regexInfo = match[1].str();
					file.close(); // Close the file
					return regexInfo;
				}
			}
		}

		file.close(); // Close the file
	}
	catch (const std::exception &ex)
	{
		std::wcerr << L"Exception: " << ex.what() << std::endl;
	}

	return L"";
}

bool CheckMusicFile(const fs::path &folderPath,
					 const fs::path &smPath,
					 const std::wregex &regex,
					 const std::wstring &regexInfo)
{
	size_t dot = regexInfo.find_last_of(L'.');
	std::wstring extension = L'.' + regexInfo.substr(dot + 1);
	for (auto &music : fs::directory_iterator(folderPath))
	{
		// Check if it is a file
		if (fs::is_regular_file(music.path()))
		{
			// Check if the file extension is ".mp3"
			if (music.path().extension() == extension)
			{
				boost::filesystem::path newFilePath = music.path().parent_path() / (L"audio" + music.path().extension().wstring());
				boost::filesystem::rename(music.path(), newFilePath);
				// std::wcout << "File renamed to: " << newFilePath << std::endl;

				// Update the .sm file with the new information (audio.mp3)
				std::wifstream smFile(smPath.wstring());
				std::wofstream newSmFile(smPath.wstring() + L".tmp");

				std::wstring line;
				while (std::getline(smFile, line))
				{
					std::wsmatch match;
					if (std::regex_search(line, match, regex))
					{
						if (match.size() > 1)
						{
							// Replace the old information with "audio.mp3"
							line = L"#MUSIC:audio" + music.path().extension().wstring() + L";";
						}
					}
					newSmFile << line << std::endl;
				}

				smFile.close();
				newSmFile.close();

				// Remove the original .sm file
				fs::remove(smPath);

				// Rename the temporary .sm file to the original name
				fs::rename(smPath.wstring() + L".tmp", smPath);

				std::wcout << ".sm file updated with new information." << std::endl;
			}
		}
	}
}

bool CheckFile(PARA_INFO &para)
{
	const std::wregex regex(L"#MUSIC:(.+?);");
	const std::wstring songPath = para.tempPath.wstring();
	fs::path folderPath(songPath);
	try
	{
		// Traverse folders
		for (auto &entry : fs::directory_iterator(folderPath))
		{
			// Check if it is a file
			if (fs::is_regular_file(entry.path()))
			{
				// Check if the file extension is ".sm"
				if (entry.path().extension() == ".sm")
				{
					//std::cout <<std::endl<< "Found .sm file: " << entry.path() << std::endl;

					// Process the .sm file
					std::wstring regexInfo = FindRegex(entry.path().wstring(), regex);
					// std::wcout << "music " << regexInfo << std::endl;
					if (!IsEnglishOrSymbol(regexInfo))
					{
						CheckMusicFile(folderPath, entry.path(), regex, regexInfo);
					}
					if(!IsEnglishOrSymbol(entry.path().filename().wstring()))
					{
						fs::path newFilePath = entry.path().parent_path() / L"simfile.sm";
						try
						{
							fs::rename(entry.path(), newFilePath);
							//std::wcout << L"File renamed to: " << newFilePath << std::endl;
						}
						catch (const std::filesystem::filesystem_error& ex)
						{
							std::wcerr << L"Error renaming file: " << ex.what() << std::endl;
							return false;
						}
					}
				}
			}
		}
	}
	catch (const fs::filesystem_error &ex)
	{
		std::wcerr << L"Error: " << ex.what() << std::endl;
		return false;
	}

	//rename folder
	if(!IsEnglishOrSymbol(para.songPath.filename().wstring()))
	{
		//translate
		std::regex pattern("[^A-Za-z0-9 _.\\-\\[\\]\\{\\}\\(\\)]");
		std::string newFolderName = Transliterate(para.songPath.filename().wstring());
		newFolderName = std::regex_replace(newFolderName, pattern, "");
		if(newFolderName.empty())
		{
			auto currentTimePoint = std::chrono::system_clock::now();
			std::time_t currentTime = std::chrono::system_clock::to_time_t(currentTimePoint);
			newFolderName = std::ctime(&currentTime);
		}
		std::cout << "newFolderName " << newFolderName << std::endl;
		//todo rename the folder, path.songPath type is boost::filesystem::wpath
		boost::filesystem::wpath newFolderPath = para.tempPath;
		newFolderPath.remove_filename();
		newFolderPath /= newFolderName;
		std::cout << "newFolderPath " << newFolderPath.string() << std::endl;
		boost::filesystem::rename(folderPath, newFolderPath);
		para.tempPath = newFolderPath;
		std::wcout << L"Folder renamed: " << folderPath << L" -> " << newFolderPath << std::endl;
	}
	return true;
}

// Function to check if a file has a video extension
bool IsVideoFile(const fs::path &filePath)
{
	static const std::vector<std::wstring> videoExtensions = {L".mp4", L".mpeg", L".avi"};

	auto extension = filePath.extension();
	std::wstring lowercasedExtension = extension.c_str(); // Convert to C-style string
	std::transform(lowercasedExtension.begin(), lowercasedExtension.end(), lowercasedExtension.begin(), towlower);

	return std::find(videoExtensions.begin(), videoExtensions.end(), lowercasedExtension) != videoExtensions.end();
}

void RemoveVideoFiles(const fs::path &directory)
{
	try
	{
		// Iterate over the files in the directory
		for (const auto &entry : fs::directory_iterator(directory))
		{
			if (fs::is_regular_file(entry.path()) && IsVideoFile(entry.path()))
			{
				// Remove the video file
				fs::remove(entry.path());
				std::wcout << L"Removed video file: " << entry.path() << std::endl;
			}
		}
	}
	catch (const std::filesystem::filesystem_error &ex)
	{
		std::wcerr << L"Error: " << ex.what() << std::endl;
	}
}

void FilterFile(const PARA_INFO para)
{
	try
	{
		// Construct the path to the "FileBackup" folder
		fs::path backupFolderPath = para.tempPath / L"FileBackup";

		// Check if the folder exists
		if (fs::exists(backupFolderPath) && fs::is_directory(backupFolderPath))
		{
			// Remove the folder and its contents
			fs::remove_all(backupFolderPath);
			//std::wcout << L"Removed folder: " << backupFolderPath << std::endl;
		}
		else
		{
			//std::wcout << L"Folder does not exist: " << backupFolderPath << std::endl;
		}
		if (para.filter)
		{
			if (fs::exists(para.tempPath) && fs::is_directory(para.tempPath))
			{
				// Remove video files from the directory
				RemoveVideoFiles(para.tempPath);
			}
			else
			{
				std::wcout << L"Directory does not exist: " << para.tempPath << std::endl;
			}
		}
	}
	catch (const std::filesystem::filesystem_error &ex)
	{
		std::wcerr << L"Error: " << ex.what() << std::endl;
	}
}

bool CompressSongDir(PARA_INFO& para)
{
	bool result = true;
	// 1. Create a connection folder
	// Connection directory path
	boost::filesystem::wpath connectDirPath = boost::filesystem::current_path() / "Songs" / "connect" / "temp";
	// Create a directory
	DeleteFolder(connectDirPath);

	//printf("path %s\n",connectDirPath.c_str());
	boost::filesystem::create_directories(connectDirPath);

	// 2. Put the song folder into the temporary folder
	result = CopyFolder(para, connectDirPath);
	if (!result) return result;

	//3.If the song information or folder is not in English, rename it
	// std::cout << "The path is: " << para.tempPath << std::endl;
	result = CheckFile(para);
	if (!result) return result;

	// 4. Filter unwanted files
	FilterFile(para);

	//5. Put it into a zip file
	//compressFolder(para.tempPath.wstring(), "output.zip");
	MyMiniZip unZip;
	boost::filesystem::wpath outputFile = connectDirPath.parent_path() / "temp.zip";
	unZip.CompressToPackageZip(para.tempPath.string(), outputFile.string());
	printf_s("Total time taken %d seconds\r\n", unZip.GetCountTime());
	DeleteFolder(connectDirPath);

	printf("CompressSongDir result: %d\n", result);
	return result;
}

void DecompressSongFolder(void)
{
	MyMiniZip unZip;
	boost::filesystem::wpath connectDirPath = boost::filesystem::current_path() / "Songs" / "connect";
	boost::filesystem::wpath zipPath = connectDirPath / "temp.zip";
	unZip.unZipPackageToLoacal(zipPath.string(), connectDirPath.string());
	printf_s("Total time taken %d seconds\r\n", unZip.GetCountTime());
}
