#include <misc.h>
#include <PathFunctions.h>
#include <BitFunctions.h>

#include <dirent.h>//TODO Windows
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iomanip>
#include <cstring>

// ONLY for library (except STL) independent miscellaneous functions

std::ostream& hex_dump(std::ostream& os, const void *buffer,  std::size_t bufsize, bool showPrintableChars)
{
    if (buffer == nullptr) {
        return os;
    }
    auto oldFormat = os.flags();
    auto oldFillChar = os.fill();
    constexpr std::size_t maxline{8};
    // create a place to store text version of string
    char renderString[maxline+1];
    char *rsptr{renderString};
    // convenience cast
    const unsigned char *buf{reinterpret_cast<const unsigned char *>(buffer)};

    for (std::size_t linecount=maxline; bufsize; --bufsize, ++buf) {
        os << std::setw(2) << std::setfill('0') << std::hex 
           << static_cast<unsigned>(*buf) << ' ';
        *rsptr++ = std::isprint(*buf) ? *buf : '.';
        if (--linecount == 0) {
            *rsptr++ = '\0';  // terminate string
            if (showPrintableChars) {
                os << " | " << renderString;
            } 
            os << '\n';
            rsptr = renderString;
            linecount = std::min(maxline, bufsize);
        }
    }
    // emit newline if we haven't already
    if (rsptr != renderString) {
        if (showPrintableChars) {
            for (*rsptr++ = '\0'; rsptr != &renderString[maxline+1]; ++rsptr) {
                 os << "   ";
            }
            os << " | " << renderString;
        }
        os << '\n';
    }

    os.fill(oldFillChar);
    os.flags(oldFormat);
    return os;
}

void runForAllFiles(const std::string& path, const std::function<void(const std::string&)>& f, bool recursive){
	//TODO: windows, see also CFileSystem::createFileList
	DIR* dirHandle=opendir(path.c_str());
	if(dirHandle){
		std::string pathWithDelimeter = appendMissingPathDelimeter(path);
		struct dirent *dirEntry;
		while((dirEntry=readdir(dirHandle))){
			bool isDirectory = false;
			if((strcmp(dirEntry->d_name, ".")==0) || (strcmp(dirEntry->d_name, "..")==0)){
				continue;
			}
			if(dirEntry->d_type == DT_UNKNOWN){
				struct stat buf;
				if (stat(dirEntry->d_name, &buf)==0){
					isDirectory = S_ISDIR(buf.st_mode);
				}
			}else{
				isDirectory = dirEntry->d_type == DT_DIR;
			}
			if(!isDirectory){
				f(pathWithDelimeter+dirEntry->d_name);
			}else if(recursive){
				runForAllFiles(pathWithDelimeter+dirEntry->d_name, f, recursive);
			}
		}
		closedir(dirHandle);
	}
}

double powSAM(double value, uint32_t power){
	double result = 1.0;
	int8_t i=31; for(; i>=0 && !getBit(power,i); i--){}//find first 1 to omit unnecessary squares of 1
	for(; i>=0; i--){
		if(getBit(power,i)){//square and multiply
			result = result * result * value;
		}else{
			result = result * result;
		}
	}
	return result;
}
