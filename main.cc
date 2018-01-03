#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <cstring>
#include <iostream>
#include <utility>
#include <unordered_set>
#include <unordered_map>
using namespace std;

#include "scx/Dir.h"
#include "scx/FileInfo.h"
using namespace scx;

#include "Tools.h"
#include "ConsUnit.h"
#include "Parallel.h"

static const unordered_set<string> C_EXT = { "c" };
static const unordered_set<string> CXX_EXT = { "cc", "cxx", "cpp", "C" };
static const unordered_set<string> ASM_EXT = { "s", "S", "asm", "nas" };

namespace Error {
    enum {
        Argument = 1,
        Permission,
        Exist,
        Empty,
        Dependency,
        Compile,
        Link
    };
}

static void Usage(const string& cmd)
{
    const string sp(cmd.size(), ' ');
    cout << ""
        "Usage:\n"
        "\t" + cmd + " [files ...] [dirs ...]\n"
        "\t" + sp + " as=?        assembler\n"
        "\t" + sp + " asflags=?   assembler flags\n"
        "\t" + sp + " cc=?        c compiler\n"
        "\t" + sp + " cflags=?    c compiler flags\n"
        "\t" + sp + " cxx=?       c++ compiler\n"
        "\t" + sp + " cxxflags=?  c++ compiler flags\n"
        "\t" + sp + " flags=?     compiler common flags\n"
        "\t" + sp + " ld=?        linker\n"
        "\t" + sp + " ldflags=?   linker flags\n"
        "\t" + sp + " ldfirst=?   anchor first object file\n"
        "\t" + sp + " jobs=?      parallel build\n"
        "\t" + sp + " workdir=?   work direcotry\n"
        "\t" + sp + " out=?       binary output file name\n"
        "\t" + sp + " prefix=?    search head file and library in\n"
        "\t" + sp + " nolink      do not link\n"
        "\t" + sp + " verbose     verbose output\n"
        "\t" + sp + " clean       clean build output\n"
        "\t" + sp + " help        show this help message\n"
        "\n"
        "\t" + sp + " thread      link against pthread\n"
        "\t" + sp + " shared      generate shared library\n"
        "\n"
        "\t" + sp + " strict      -Wall -Wextra -Werror\n"
        "\t" + sp + " release     -DNDEBUG\n"
        "\t" + sp + " debug       -g\n"
        "\t" + sp + " c++11       -std=c++11\n"
        "\t" + sp + " c++14       -std=c++14\n"
        "\t" + sp + " c++1y       -std=c++1y\n"
        "\t" + sp + " c++1z       -std=c++1z\n"
        "\n";

    cout << ""
        "Contact:\n"
        "\tYanhui Shen <@bsdelf on Twitter>\n";
}

int main(int argc, char** argv)
{
    unordered_map<string, string> ArgTable = {
        {   "as",       "as"    },
        {   "asflags",  ""      },
        {   "cc",       "cc"    },
        {   "cflags",   ""      },
        {   "cxx",      "c++"   },
        {   "cxxflags", ""      },
        {   "flags",    ""      },
        {   "ld",       "cc"    },
        {   "ldflags",  ""      },
        {   "ldfirst",  ""      },
        {   "jobs",     "0"     },
        {   "workdir",  ""      },
        {   "out",      "b.out" }
    };

    bool clean = false;
    bool nolink = false;
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
        bool useC11 = false;
        bool useCXX11 = false;
        bool useCXX14 = true;
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
                return EXIT_SUCCESS;
            } else if (arg == "verbose") {
                verbose = true;
            } else if (arg == "nolink") {
                nolink = true;
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
            } else if (arg == "c11") {
                useC11 = true;
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
                        auto files = Dir::ListDir(arg);
                        std::transform(files.begin(), files.end(), files.begin(), [&arg](const std::string& file) {
                            return arg + "/" + file;
                        });
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
                        return Error::Argument;
                    }
                    break;
                }
            }
        }

        if (!ArgTable["workdir"].empty()) {
            auto& dir = ArgTable["workdir"];
            if (dir[dir.size()-1] != '/') {
                dir += "/";
            }
            const auto& info = FileInfo(dir);
            if (!info.Exists()) {
                if (!Dir::MakeDir(dir, 0744)) {
                    cerr << "Failed to create directory!" << endl;
                    cerr << "    Directory: " << dir << endl;
                    return Error::Permission;
                }
            } else if (info.Type() != FileType::Directory) {
                cerr << "Bad work directory! " << endl;
                cerr << "    Directory: " << dir << endl;
                cerr << "    File type: " << FileType::ToString(info.Type()) << endl;
                return Error::Exist;
            }
        }

        //-------------------------------------------------------------
        // additional compiler common flags

        if (!ArgTable["flags"].empty()) {
            ArgTable["flags"].insert(0, 1, ' ');
        }

        for (const auto& prefix: prefixes) {
            ArgTable["flags"] += " -I " + prefix + "/include";
            ArgTable["ldflags"] += " -L " + prefix + "/lib";
        }

        if (useStrict) {
            ArgTable["flags"] += " -Wall -Wextra -Werror";
        }

        if (useRelease) {
            ArgTable["flags"] += " -DNDEBUG";
        } else if (useDebug) {
            ArgTable["flags"] += " -g";
        }

        if (usePipe) {
            ArgTable["flags"] += " -pipe";
        }

        if (useShared) {
            ArgTable["flags"] += " -fPIC";
            ArgTable["ldflags"] += " -shared";
        }

        //-------------------------------------------------------------
        // additional link flags

        if (!ArgTable["ldflags"].empty()) {
            ArgTable["ldflags"].insert(0, 1, ' ');
        }

        if (useThread) {
            ArgTable["flags"] += " -pthread";
#ifndef __APPLE__
            ArgTable["ldflags"] += " -pthread";
#endif
        }
        
        //-------------------------------------------------------------
        // additional c/c++ compiler flags

        if (ArgTable["cflags"].empty()) {
            ArgTable["cflags"] = ArgTable["flags"];
        } else {
            ArgTable["cflags"].insert(0, ArgTable["flags"] + " ");
        }

        if (useC89) {
            ArgTable["cflags"] += " -std=c89";
        } else if (useC99) {
            ArgTable["cflags"] += " -std=c99";
        } else if (useC11) {
            ArgTable["cflags"] += " -std=c11";
        }

        if (ArgTable["cxxflags"].empty()) {
            ArgTable["cxxflags"] = ArgTable["flags"];
        } else {
            ArgTable["cxxflags"].insert(0, ArgTable["flags"] + " ");
        }

        if (useCXX11) {
            ArgTable["cxxflags"] += " -std=c++11";
        } else if (useCXX14) {
            ArgTable["cxxflags"] += " -std=c++14";
        } else if (useCXX1y) {
            ArgTable["cxxflags"] += " -std=c++1y";
        } else if (useCXX1z) {
            ArgTable["cxxflags"] += " -std=c++1z";
        }

        //-------------------------------------------------------------

        // if unspecified, take all files under current folder
        if (allsrc.empty()) {
            allsrc = Dir::ListDir(".");
        }

        if (allsrc.empty()) {
            cerr << "FATAL: nothing to build!" << endl;
            return Error::Empty;
        }

        // sort by file path
        std::sort(allsrc.begin(), allsrc.end(), [](const std::string& str1, const std::string& str2) {
            return std::strcoll(str1.c_str(), str2.c_str()) < 0 ? true : false;
        });
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
            compilerFlag = ArgTable["cflags"];
            ok = ConsUnit::InitC(unit, compiler, compilerFlag);
        } else if (CXX_EXT.find(ext) != CXX_EXT.end()) {
            hasCpp = true;
            compiler = ArgTable["cxx"];
            compilerFlag = ArgTable["cxxflags"];
            ok = ConsUnit::InitCpp(unit, compiler, compilerFlag);
        } else if (ASM_EXT.find(ext) != ASM_EXT.end()) {
            compiler = ArgTable["as"];
            compilerFlag = ArgTable["asflags"];
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
            cerr << "    flags:    " << ArgTable["flags"] << endl;
            cerr << "    cflags:   " << ArgTable["cflags"] << endl;
            cerr << "    cxxflags: " << ArgTable["cxxflags"] << endl;
            return Error::Dependency;
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
                for (const auto& dep: unit.deps) {
                    cout << dep << ", ";
                }
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
        return EXIT_SUCCESS;
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
            return Error::Compile;
        }
    }

    // link
    if ((!hasOut || !newUnits.empty()) && !nolink) {
        string ldCmd = ArgTable["ld"] + ArgTable["ldflags"];
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
            cerr << "    ldflags: " << ArgTable["ldflags"] << endl;
            return Error::Compile;
        }
    }

    return EXIT_SUCCESS;
}
