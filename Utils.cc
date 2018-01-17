#include "Utils.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <regex>

// since Apple doesn't complaint with POSIX:2008
#ifdef __APPLE__
#define st_atim		st_atimespec
#define st_mtim		st_mtimespec
#define st_ctim		st_ctimespec
#define st_birthtim	st_birthtimespec
#endif

auto DoCmd(const std::string& cmd) -> std::string
{
    std::string output;

    FILE* pipe = ::popen(cmd.c_str(), "r");
    if (pipe) {
        while (!::feof(pipe)) {
            char chunk[1024];
            if (::fgets(chunk, sizeof(chunk), pipe) != nullptr) {
                output += chunk;
            }
        }
        ::pclose(pipe);
    }

    return output;
}

auto RegexSplit(const std::string& str, const std::string& pattern) -> std::vector<std::string>
{
    std::regex re { pattern };
    std::sregex_token_iterator
        first { str.begin(), str.end(), re, -1 },
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
