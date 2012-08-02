#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <future>
#include <atomic>
using namespace std;

#include "scx/Dir.hpp"
#include "scx/FileInfo.hpp"
using namespace scx;

#include "Tools.h"
#include "EasyBuild.h"

const unordered_set<string> C_EXT = { "c" };
const unordered_set<string> CXX_EXT = { "cc", "cxx", "cpp", "C" };

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
    // TODO: Parallel build.
    if (!clean) {
        bool compiled = false;
        string ldCmd = ArgTable["ld"] + " -o " + ArgTable["out"] + " " +
                       ArgTable["flag"] + " " + ArgTable["ldflag"] + " ";

        if (shared)
            ldCmd += "-shared ";

        cout << "== Compile ==" << endl;
        size_t buildIndex = 0;
        for (const auto& unit: units) {
            // If build command isn't empty, do it.
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

            // Record all our objects.
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
