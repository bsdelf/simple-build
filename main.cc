#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
using namespace std;

#include "scx/Dir.hpp"
#include "scx/FileInfo.hpp"
using namespace scx;

#include "Tools.h"
#include "ConsUnit.h"
#include "Parallel.h"

static const unordered_set<string> C_EXT = { "c" };
static const unordered_set<string> CXX_EXT = { "cc", "cxx", "cpp", "C" };

static void Usage(const string& cmd)
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
        "\t" + sp + " jobs=?      Parallel build.\n"
        "\t" + sp + " clean       Clean build output.\n"
        "\t" + sp + " help        Show this help message.\n"
        "\n"
        "\t" + sp + " shared      Generate shared library. (*)\n"
        "\t" + sp + " debug       Build with debug symbols. (*)\n"
        "\t" + sp + " c++11       Use clang & libc++. (*)\n"
        "\t" + sp + " thread      Link against pthread. (*)\n"
        "\n";

    cout << ""
        "Note:\n"
        "\t* Just for convenient. You may also use flags to approach the function.\n"
        "\n";

    cout << ""
        "Author:\n"
        "\tYanhui Shen <@diffcat on Twitter>\n";
}

int main(int argc, char** argv)
{
    unordered_map<string, string> ArgTable = {
        { "out", "a.exe" },
        { "with", "" },
        { "without", "" },
        { "cc", "cc" },
        { "cxx", "c++" },
        { "flag", "" },
        { "ld", "cc" },
        { "ldflag", "" },
        { "jobs", "0" }
    };

    bool shared = false;
    bool debug = false;
    bool clean = false;
    bool useClangXX11 = false;
    bool useThread = false;
    vector<string> with, without, all;

    // Parse arguments.
    for (int i = 1; i < argc; ++i) {
        const string& arg(argv[i]);

        // So obvisously, file name should not contain '='.
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
        } else if (arg == "c++11") {
            useClangXX11 = true;
        } else if (arg == "thread") {
            useThread = true;
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

    if (useThread) {
        ArgTable["ldflag"] += "-pthread";
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
        if ( ConsUnit::Init(unit, compiler, flag) ) {
            units.push_back(unit);
            if ( !unit.build.empty() ) ++buildCount;
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
        cout << "== Compile ==" << endl;
        ParallelCompiler pc(units, buildCount);
        if (pc.Run( stoi(ArgTable["jobs"]) ) != 0)
            return 1;

        cout << "== Generate ==" << endl;
        string ldCmd = ArgTable["ld"] + " -o " + ArgTable["out"] + " " +
                       ArgTable["flag"] + " " + ArgTable["ldflag"] + " " + 
                       pc.Objects() + " ";

        if (shared)
            ldCmd += "-shared ";

        if ( (buildCount != 0) || !FileInfo(ArgTable["out"]).Exists() ) {
            cout << ldCmd << endl;
            if (::system( ldCmd.c_str() ) != 0)
                return 1;
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
