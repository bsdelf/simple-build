#include "Tools.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <regex>
using namespace std;

// since Apple doesn't complaint with POSIX:2008
#ifdef __APPLE__
#define st_atim		st_atimespec
#define st_mtim		st_mtimespec
#define st_ctim		st_ctimespec
#define st_birthtim	st_birthtimespec
#endif

auto DoCmd(const std::string& cmd) -> std::string 
{
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) 
        return "";

    std::string output;
    char buffer[1024];
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != NULL)
            output += buffer;
    }
    pclose(pipe);

    return output;
}

auto RegexSplit(const std::string& in, const std::string& sre) -> std::vector<std::string>
{
    std::regex re(sre);
    std::sregex_token_iterator
        first { in.begin(), in.end(), re, -1 },
        last;
    return { first, last };
}

auto IsNewer(const std::string& f1, const std::string& f2) -> bool
{
    struct stat s1, s2;
    ::memset(&s1, 0, sizeof(struct stat));
    ::memset(&s2, 0, sizeof(struct stat));

    ::stat(f1.c_str(), &s1);
    ::stat(f2.c_str(), &s2);
    return ((s1.st_mtim.tv_sec == s2.st_mtim.tv_sec) ?
            (s1.st_mtim.tv_nsec > s2.st_mtim.tv_nsec) :
            (s1.st_mtim.tv_sec > s2.st_mtim.tv_sec));
}
