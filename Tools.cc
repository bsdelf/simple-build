#include "Tools.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <regex>
using namespace std;

std::string DoCmd(const std::string& cmd)
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

std::vector<std::string> RegexSplit(const std::string& in, const std::string& sre)
{
    std::regex re(sre);
    std::sregex_token_iterator
        first { in.begin(), in.end(), re, -1 },
        last;
    return { first, last };
}

bool IsNewer(const std::string& f1, const std::string& f2)
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
