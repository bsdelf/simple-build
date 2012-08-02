#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <regex>
#include <future>
#include <atomic>
using namespace std;

#include "scx/Dir.hpp"
#include "scx/FileInfo.hpp"
using namespace scx;

const unordered_set<string> C_EXT = { "c" };
const unordered_set<string> CXX_EXT = { "cc", "cxx", "cpp", "C" };

struct ConsUnit
{
    string in;
    string out;
    string build;
#ifdef DEBUG
    vector<string> deps;
#endif

    explicit ConsUnit(const string& _in): in(_in) { }
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

// Maybe we can cache mtime to reduce the stat() system calls.
bool InitConstUnit(ConsUnit& u, const string& c, const string& f)
{
    const string& cmd(c + " -MM " + u.in + " " + f);
    const string& deps( DoCmd(cmd) );
    const string& re("(\\s)+(\\\\)*(\\s)*");
    
    const auto& l = RegexSplit(deps, re);
    if (l.size() >= 2) {
        u.out = l[0].substr(0, l[0].size()-1);

        bool need = false;
        if ( FileInfo(u.out).Exists() ) {
            for (size_t i = 1; i < l.size(); ++i) {
                if (IsNewer(l[i], u.out)) {
                    need = true;
                    break;
                }
            }
        } else {
            need = true;
        }

        if (need) {
            u.build = c + " -o " + u.out + " -c " + u.in + " " + f;
#ifdef DEBUG
            u.deps.assign(l.begin()+1, l.end());
#endif
        }
        return true;
    } else {
        return false;
    }
}

void Usage(const string& cmd)
{
    const string sp(cmd.size(), ' ');
    cout << ""
        "Usage:\n"
        "\t" + cmd + " [ file1 file1 | dir1 dir2 ]\n"
        "\t" + sp + " cc=?        C compiler.\n"
        "\t" + sp + " cxx=?       C++ compiler.\n"
        "\t" + sp + " flag=?      Compiler flag.\n"
        "\t" + sp + " ldflag=?    Linker flag.\n"
        //"   " + sp + " with=?,?,? without=?,?,?\n"
        //"   " + sp + " obj=\"outA:dep1, dep2:cmdA; outB:dep1, dep2:cmdB; ...\"\n"
        "\t" + sp + " clean       Clean build output.\n"
        "\t" + sp + " help        Show this help message.\n"
        "\n"
        "\t" + sp + " shared      Generate shared library. (*)\n"
        "\t" + sp + " debug       Build with debug symbols. (*)\n"
        "\t" + sp + " clang++11   Use clang & libc++. (*)\n"
        "\n";

    cout << ""
        "Note:\n"
        "\t(*) Just for convenient. You may also use flags to approach the function.\n"
        "\n";

    cout << ""
        "Author:\n"
        "\tYanhui Shen <@pagedir on Twitter>\n";
}

int main(int argc, char** argv)
{
    const unsigned int cpus = sysconf(_SC_NPROCESSORS_ONLN);

    unordered_map<string, string> ArgTable = {
        { "out", "a.exe" },
        { "with", "" },
        { "without", "" },
        { "cc", "cc" },
        { "cxx", "c++" },
        { "flag", "" },
        { "ld", "cc" },
        { "ldflag", "" }
    };

    bool shared = false;
    bool debug = false;
    bool clean = false;
    bool useClangXX11 = false;
    vector<string> with, without, all;

    // Parse arguments.
    for (int i = 1; i < argc; ++i) {
        const string& arg(argv[i]);

        // So obvisously, file name should not contains '='.
        size_t pos = arg.find('=');
        if (pos != string::npos && pos != arg.size()-1) {
            // Update key-value.
            auto iter = ArgTable.find( arg.substr(0, pos) );
            if (iter != ArgTable.end()) {
                iter->second = arg.substr(pos+1);
            }         
        } else if (arg == "clean") {
            clean = true;
        } else if (arg == "help") {
            Usage(argv[0]);
            return 0;
        } else if (arg == "debug") {
            debug = true;
        } else if (arg == "shared") {
            shared = true;
        } else if (arg == "clang++11") {
            useClangXX11 = true;
        } else {
            switch ( FileInfo(arg).Type() ) {
            case FileType::Directory:
            {
                auto files = Dir::ListTree(arg);
                all.reserve(all.size() + files.size());
                all.insert(all.end(), files.begin(), files.end());
            }
                break;

            case FileType::Regular:
            {
                all.push_back(arg);
            }
                break;

            default:
            {
                cout << "WARN: bad argument." << endl;
                cout << "\targ:" << arg << endl;
            }
                break;
            }
        }
    }

    if (useClangXX11) {
        ArgTable["cc"] = "clang";
        ArgTable["cxx"] = "clang++";
        ArgTable["flag"] += " -std=c++11 -stdlib=libc++ ";
    }

    if (all.empty())
        all = Dir::ListTree(".");
    if (all.empty()) {
        cerr << "FATAL: nothing to build!" << endl;
        return 1;
    }

    // Prepare construct units.
    bool hasCpp = false;
    vector<ConsUnit> units;
    size_t buildCount = 0;
    for (const auto& file: all) {
        ConsUnit unit(file);
        string compiler, flag;

        if (debug)
            flag += "-g ";
        if (shared)
            flag += "-fPIC ";

        // Pick up source file.
        const string ext( FileInfo(file).Suffix() );
        if (C_EXT.find(ext) != C_EXT.end()) {
            compiler = ArgTable["cc"];
            flag += ArgTable["flag"];
        } else if (CXX_EXT.find(ext) != CXX_EXT.end()) {
            compiler = ArgTable["cxx"];
            flag += ArgTable["flag"];
            hasCpp = true;
        } else
            continue;

        // Calculate dependence.
        if ( InitConstUnit(unit, compiler, flag) ) {
            units.push_back(unit);
            if (!unit.build.empty()) ++buildCount;
        } else {
            cerr << "FATAL: failed to calculate dependence!" << endl;
            cerr << "\tfile: " << file << endl;
            cerr << "\tcompiler:" << compiler << endl;
            cerr << "\tflag:" << flag << endl;
            return 1;
        }
    }
    if (hasCpp)
        ArgTable["ld"] = ArgTable["cxx"];

#ifdef DEBUG
    // Debug info.
    for (const auto& unit: units) {
        cout << "in: " <<  unit.in << ", " << "out: " << unit.out << endl;
        cout << "\t" << unit.build << endl;
        cout << "\t";
        for (const auto& dep: unit.deps)
            cout << dep << ", ";
        cout << endl;
    }
#endif

    // Let's build them all.
    if (!clean) {
        bool compiled = false;
        string ldCmd = ArgTable["ld"] + " -o " + ArgTable["out"] + " " +
                       ArgTable["flag"] + " " + ArgTable["ldflag"] + " ";

        if (shared)
            ldCmd += "-shared ";

        cout << "== Compile ==" << endl;
        size_t buildIndex = 0;
        for (const auto& unit: units) {
            if (!unit.build.empty()) {
                assert(buildCount != 0);

                char buf[] = "[ 100% ] ";
                int percent = (double)(++buildIndex) / buildCount * 100;
                ::snprintf(buf, sizeof buf, "[ %3.d%% ] ", percent);
                cout << buf << unit.build << endl;

                if (::system( unit.build.c_str() ) != 0)
                    return 2;
                compiled = true;
            }

            ldCmd += unit.out + " ";
        }

        cout << "== Generate ==" << endl;
        if (compiled || !FileInfo(ArgTable["out"]).Exists() ) {
            cout << ldCmd << endl;
            if (::system( ldCmd.c_str() ) != 0)
                return 3;
        }

        cout << "== Done! ==" << endl;
    } else {
        const string& rm("rm -f ");
        for (const auto& unit: units) {
            const string& cmd(rm + unit.out);
            cout << cmd << endl;
            ::system( cmd.c_str() );
        }
        const string& cmd(rm + ArgTable["out"]);
        cout << cmd << endl;
        ::system( cmd.c_str() );
    }

    return 0;
}
