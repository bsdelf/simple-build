#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string>
#include <iostream>
#include <unordered_set>
#include <regex>
#include <future>
#include <atomic>
using namespace std;

#include "scx/Dir.hpp"
#include "scx/FileInfo.hpp"
using namespace scx;

const char* const CC = "clang";
const char* const CXX = "clang++";
const char* const CXX_FLAG = "-std=c++11 -stdlib=libc++";
const char* const LD = "clang++";
const char* const LD_FLAG = "-lc++";
const unordered_set<string> C_EXT = { "c" };
const unordered_set<string> CXX_EXT = { "cc", "cxx", "cpp", "C" };

struct ConsUnit
{
    string in;

    string out;
    string build;
    vector<string> deps;
};

std::string DoCmd(const string& cmd)
{
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) 
        return "";

    std::string output;
    char buffer[1024];
    while ( !feof(pipe) ) {
        if (fgets(buffer, sizeof(buffer), pipe) != NULL)
            output += buffer;
    }
    pclose(pipe);

    return output;
}

std::vector<std::string> RegexSplit(const string& in, const string& sre)
{
    std::regex re(sre);
    std::sregex_token_iterator
        first { in.begin(), in.end(), re, -1 },
        last;
    return { first, last };
}

bool IsNewer(const string& f1, const string& f2)
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

bool CalcDep(ConsUnit& u, const string& c, const string& f)
{
    const string& cmd(c + " -MM " + u.in + " " + f);
    string deps( DoCmd(cmd) );
    string re("(\\s)+(\\\\)*(\\s)*");
    
    auto l = RegexSplit(deps, re);
    if (l.size() >= 2) {
        u.out = l[0].substr(0, l[0].size()-1);
        u.build = c + " -o " + u.out + " -c " + u.in + " " + f;
        u.deps.assign(l.begin()+1, l.end());
        return true;
    } else
        return false;
}

int main(int argc, char** argv)
{
    /*
     * USAGE:
     * eb (dir) cc=? cxx=? ccflag=? cxxflag=? ld=? 
     *          with=?;?;? without=?;?;? obj=?;?;? clean force
     */

    const string prjRoot(argc >= 2 ? argv[1] : ".");

    // Prepare construct units.
    vector<ConsUnit> units;
    for (auto file: Dir::ListTree(prjRoot)) {
        ConsUnit unit { file };
        string compiler, flag;

        // Pick up source file.
        const string ext( FileInfo(file).Suffix() );
        if (C_EXT.find(ext) != C_EXT.end()) {
            compiler = CC;
        } else if (CXX_EXT.find(ext) != CXX_EXT.end()) {
            compiler = CXX;
            flag = CXX_FLAG;
        } else
            continue;

        // Calculate dependence.
        if (CalcDep(unit, compiler, flag)) {
            units.push_back(unit);
        } else {
            cerr << "FATAL: failed to calculate dependence!" << endl;
            cerr << "\tfile: " << file << endl;
            cerr << "\tcompiler:" << compiler << endl;
            cerr << "\tflag:" << flag << endl;
            return 1;
        }
    }

#ifdef DEBUG
    // Debug info.
    for (auto unit: units) {
        cout << "in: " <<  unit.in << ", " << "out: " << unit.out << endl;
        cout << "\t" << unit.build << endl;
        cout << "\t";
        for (auto dep: unit.deps)
            cout << dep << ", ";
        cout << endl;
    }
#endif

    // Let's build them all.
    unsigned int cpus = sysconf(_SC_NPROCESSORS_ONLN);

    string output = "app";
    bool hasNew = false;
    string ldCmd = LD + string(" -o ") + output + " " + string(LD_FLAG) + " ";

    cout << "== Compile All ==" << endl;
    for (auto unit: units) {
        bool isDepNewer = false;
        for (auto dep: unit.deps) {
            if (IsNewer(dep, unit.out)) {
                isDepNewer = true;
                break;
            }
        }
        if ( !FileInfo(unit.out).Exists() || isDepNewer) {
            cout << unit.build << endl;
            DoCmd(unit.build);
            hasNew = true;
        }
        ldCmd += unit.out + " ";
    }

    cout << "== Generate Output ==" << endl;
    if (hasNew || !FileInfo(output).Exists() ) {
        cout << ldCmd << endl;
        DoCmd(ldCmd);
    }

    cout << "== Done! ==" << endl;

    return 0;
}
