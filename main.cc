#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <utility>
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
static const unordered_set<string> ASM_EXT = { "s", "S", "asm", "nas" };

static void Usage(const string& cmd)
{
    const string sp(cmd.size(), ' ');
    cout << ""
        "Usage:\n"
        "\t" + cmd + " [ file1 file2 ... | dir1 dir2 ... ]\n"
        "\t" + sp + " as=?        assembler\n"
        "\t" + sp + " asflag=?    assembler flag\n"
        "\t" + sp + " cc=?        c compiler\n"
        "\t" + sp + " ccflag=?    c compiler flag\n"
        "\t" + sp + " cxx=?       c++ compiler\n"
        "\t" + sp + " cxxflag=?   c++ compiler flag\n"
        "\t" + sp + " flag=?      compiler common flag\n"
        "\t" + sp + " ld=?        linker\n"
        "\t" + sp + " ldflag=?    linker flag\n"
        "\t" + sp + " ldfirst=?   anchor first object file\n"
        "\t" + sp + " jobs=?      parallel build\n"
        "\t" + sp + " workdir=?   work direcotry\n"
        "\t" + sp + " out=?       binary output file name\n"
        "\t" + sp + " prefix=?    search head file and library in\n"
        "\t" + sp + " nlink       do not link\n"
        "\t" + sp + " verbose     verbose output\n"
        "\t" + sp + " clean       clean build output\n"
        "\t" + sp + " help        show this help message\n"
        "\n"
        "\t" + sp + " thread      link against pthread\n"
        "\t" + sp + " shared      generate shared library\n"
        "\n"
        "\t" + sp + " strict      -Wall -Werror\n"
        "\t" + sp + " release     -DNDEBUG\n"
        "\t" + sp + " debug       -g\n"
        "\t" + sp + " c++11       -std=c++11\n"
        "\t" + sp + " c++14       -std=c++14\n"
        "\t" + sp + " c++1y       -std=c++1y\n"
        "\t" + sp + " c++1z       -std=c++1z\n"
        "\n";

    cout << ""
        "Author:\n"
        "\tYanhui Shen <@bsdelf on Twitter>\n";
}

int main(int argc, char** argv)
{
    unordered_map<string, string> ArgTable = {
        {   "as",       "as"    },
        {   "asflag",   ""      },
        {   "cc",       "cc"    },
        {   "ccflag",   ""      },
        {   "cxx",      "c++"   },
        {   "cxxflag",  ""      },
        {   "flag",     ""      },
        {   "ld",       "cc"    },
        {   "ldflag",   ""      },
        {   "ldfirst",  ""      },
        {   "jobs",     "0"     },
        {   "workdir",  ""      },
        {   "out",      "b.out" }
    };

    bool clean = false;
    bool nlink = false;
    bool verbose = false;
    vector<string> allsrc;
    vector<string> prefixes;

    // Parse arguments.
    {
        bool useThread = false;
        bool useShared = false;
        bool useRelease = false;
        bool useDebug = false;
        bool useStrict = false;
        bool usePipe = true;
        bool useC89 = false;
        bool useC99 = true;
        bool useCXX11 = false;
        bool useCXX14 = false;
        bool useCXX1y = false;
        bool useCXX1z = false;

        for (int i = 1; i < argc; ++i) {
            const string& arg(argv[i]);

            // So obvisously, file name should not contain '='.
            size_t pos = arg.find('=');
            if (pos != string::npos && pos != arg.size()-1) {
                const auto& key = arg.substr(0, pos);
                const auto& val = arg.substr(pos+1);
                // Update key-value.
                auto iter = ArgTable.find(key);
                if (iter != ArgTable.end()) {
                    iter->second = val;
                } else if (key == "prefix") {
                    prefixes.push_back(val);
                } else {
                    cout << "Argument ignored!" << endl;
                    cout << "    Argument: " << arg << endl;
                }
            } else if (arg == "help") {
                Usage(argv[0]);
                return 0;
            } else if (arg == "verbose") {
                verbose = true;
            } else if (arg == "nlink") {
                nlink = true;
            } else if (arg == "clean") {
                clean = true;
            } else if (arg == "thread") {
                useThread = true;
            } else if (arg == "strict") {
                useStrict = true;
            } else if (arg == "release") {
                useRelease = true;
            } else if (arg == "debug") {
                useDebug = true;
            } else if (arg == "shared") {
                useShared = true;
            } else if (arg == "c89") {
                useC89 = true;
            } else if (arg == "c99") {
                useC99 = true;
            } else if (arg == "c++11") {
                useCXX11 = true;
            } else if (arg == "c++14") {
                useCXX14 = true;
            } else if (arg == "c++1y") {
                useCXX1y = true;
            } else if (arg == "c++1z") {
                useCXX1z = true;
            } else {
                // Add source files
                switch (FileInfo(arg).Type()) {
                case FileType::Directory:
                    {
                        const auto& files = Dir::ListDir(arg);
                        allsrc.reserve(allsrc.size() + files.size());
                        allsrc.insert(allsrc.end(), files.begin(), files.end());
                    }
                    break;

                case FileType::Regular:
                    {
                        allsrc.push_back(arg);
                    }
                    break;

                default:
                    {
                        cerr << "FATAL: bad argument: " << endl;
                        cerr << "arg: " << arg << endl;
                        return -1;
                    }
                    break;
                }
            }
        }

        if (!ArgTable["workdir"].empty()) {
            auto& dir = ArgTable["workdir"];
            if (dir[dir.size()-1] != '/')
                dir += "/";
            const auto& info = FileInfo(dir);
            if (!info.Exists()) {
                if (!Dir::MakeDir(dir, 0744)) {
                    cerr << "Failed to create directory!" << endl;
                    cerr << "    Directory: " << dir << endl;
                    return -1;
                }
            } else if (info.Type() != FileType::Directory) {
                cerr << "Bad work directory! " << endl;
                cerr << "    Directory: " << dir << endl;
                cerr << "    File type: " << FileType::ToString(info.Type()) << endl;
                return -1;
            }
        }

        //-------------------------------------------------------------
        // additional compiler common flag

        if (!ArgTable["flag"].empty()) {
            ArgTable["flag"].insert(0, 1, ' ');
        }

        for (const auto& prefix: prefixes) {
            ArgTable["flag"] += " -I " + prefix + "/include";
            ArgTable["ldflag"] += " -L " + prefix + "/lib";
        }

        if (useStrict) {
            ArgTable["flag"] += " -Wall -Werror";
        }

        if (useRelease) {
            ArgTable["flag"] += " -DNDEBUG";
        } else if (useDebug) {
            ArgTable["flag"] += " -g";
        }

        if (usePipe) {
            ArgTable["flag"] += " -pipe";
        }

        if (useShared) {
            ArgTable["flag"] += " -fPIC";
            ArgTable["ldflag"] += " -shared";
        }

        //-------------------------------------------------------------
        // additional link flag

        if (!ArgTable["ldflag"].empty()) {
            ArgTable["ldflag"].insert(0, 1, ' ');
        }

        if (useThread) {
#ifndef __APPLE__
            ArgTable["ldflag"] += " -pthread";
#endif
        }
        
        //-------------------------------------------------------------
        // additional c/c++ compiler flag

        if (ArgTable["ccflag"].empty()) {
            ArgTable["ccflag"] = ArgTable["flag"];
        } else {
            ArgTable["ccflag"].insert(0, ArgTable["flag"] + " ");
        }

        if (useC89) {
            ArgTable["ccflag"] += " -std=c89";
        } else if (useC99) {
            ArgTable["ccflag"] += " -std=c99";
        }

        if (ArgTable["cxxflag"].empty()) {
            ArgTable["cxxflag"] = ArgTable["flag"];
        } else {
            ArgTable["cxxflag"].insert(0, ArgTable["flag"] + " ");
        }

        if (useCXX11) {
            ArgTable["cxxflag"] += " -std=c++11";
        } else if (useCXX14) {
            ArgTable["cxxflag"] += " -std=c++14";
        } else if (useCXX1y) {
            ArgTable["cxxflag"] += " -std=c++1y";
        } else if (useCXX1z) {
            ArgTable["cxxflag"] += " -std=c++1z";
        }

        //-------------------------------------------------------------

        // if unspecified, take all files under current folder
        if (allsrc.empty()) {
            allsrc = Dir::ListDir(".");
        }

        if (allsrc.empty()) {
            cerr << "FATAL: nothing to build!" << endl;
            return -1;
        }
    }
    
    // Prepare construct units.
    // Note: "workdir" & "out" should occur together
    bool hasCpp = false;
    bool hasOut = FileInfo(ArgTable["workdir"] + ArgTable["out"]).Exists();
    vector<ConsUnit> newUnits;
    string allObjects;

    for (const auto& file: allsrc) {
        ConsUnit unit(ArgTable["workdir"], file);
        string compiler;
        string compilerFlag;

        // Calculate dependence.
        bool ok = false;
        const string ext(FileInfo(file).Suffix());
        if (C_EXT.find(ext) != C_EXT.end()) {
            compiler = ArgTable["cc"];
            compilerFlag = ArgTable["ccflag"];
            ok = ConsUnit::InitC(unit, compiler, compilerFlag);
        } else if (CXX_EXT.find(ext) != CXX_EXT.end()) {
            hasCpp = true;
            compiler = ArgTable["cxx"];
            compilerFlag = ArgTable["cxxflag"];
            ok = ConsUnit::InitCpp(unit, compiler, compilerFlag);
        } else if (ASM_EXT.find(ext) != ASM_EXT.end()) {
            compiler = ArgTable["as"];
            compilerFlag = ArgTable["asflag"];
            ok = ConsUnit::InitAsm(unit, compiler, compilerFlag);
        } else {
            continue;
        }

        if (ok) {
            if (unit.out == ArgTable["ldfirst"]) {
                allObjects = " " + unit.out + allObjects;
            } else {
                allObjects += " " + unit.out;
            }
            if (!unit.cmd.empty()) {
                newUnits.push_back(std::move(unit));
            }
        } else {
            cerr << "FATAL: failed to calculate dependence!" << endl;
            cerr << "    file:     " << file << endl;
            cerr << "    compiler: " << compiler << endl;
            cerr << "    flag:     " << ArgTable["flag"] << endl;
            cerr << "    ccflag:   " << ArgTable["ccflag"] << endl;
            cerr << "    cxxflag:  " << ArgTable["cxxflag"] << endl;
            return -1;
        }
    }

    if (hasCpp) {
        ArgTable["ld"] = ArgTable["cxx"];
    }

#ifdef DEBUG
    // Debug info.
    {
        auto fn = [](const vector<ConsUnit>& units) {
            for (const auto& unit: units) {
                cout << "in: " <<  unit.in << ", " << "out: " << unit.out << endl;
                cout << "\t" << unit.cmd << endl;
                cout << "\t";
                for (const auto& dep: unit.deps)
                    cout << dep << ", ";
                cout << endl;
            }
        };
        cout << "new units: " << endl;
        fn(newUnits);
    }
#endif

    // clean
    if (clean) {
        const string& cmd = "rm -f " + ArgTable["workdir"] + ArgTable["out"] + allObjects;
        cout << cmd << endl;
        ::system(cmd.c_str());
        return 0;
    }

    // compile
    if (!newUnits.empty()) {
        cout << "* Build: ";
        if (!verbose) {
            cout << newUnits.size() << (newUnits.size() > 1 ? " files" : " file");
        } else {
            for (size_t i = 0; i < newUnits.size(); ++i) {
                cout << newUnits[i].in << ((i+1 < newUnits.size()) ? ", " : "");
            }
        }
        cout << endl;

        ParallelCompiler pc(newUnits);
        pc.SetVerbose(verbose);
        if (pc.Run(std::stoi(ArgTable["jobs"])) != 0) {
            return -1;
        }
    }

    // link
    if ((!hasOut || !newUnits.empty()) && !nlink) {
        string ldCmd = ArgTable["ld"] + ArgTable["ldflag"];
        ldCmd += " -o " + ArgTable["workdir"] + ArgTable["out"] + allObjects;

        if (!verbose) {
            cout << "- Link - " << ArgTable["workdir"] + ArgTable["out"] << endl;
        } else {
            cout << "- Link - " << ldCmd << endl;
        }
        if (::system(ldCmd.c_str()) != 0) {
            cerr << "FATAL: failed to link!" << endl;
            cerr << "    file:   " << allObjects << endl;
            cerr << "    ld:     " << ArgTable["ld"] << endl;
            cerr << "    ldflag: " << ArgTable["ldflag"] << endl;
            return -2;
        }
    }

    return 0;
}
