#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <cstring>
#include <iostream>
#include <utility>
#include <unordered_map>
using namespace std;

#include "scx/Dir.h"
#include "scx/FileInfo.h"
using namespace scx;

#include "Tools.h"
#include "TransUnit.h"
#include "Compiler.h"

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
        "\n"
        "\t" + sp + " nolink      do not link\n"
        "\t" + sp + " verbose     verbose output\n"
        "\t" + sp + " clean       clean build output\n"
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
        "\n"
        "\t" + sp + " help        show this help message\n"
        "\n";

    cout << ""
        "Contact:\n"
        "\tYanhui Shen <@bsdelf on Twitter>\n";
}

int main(int argc, char* argv[])
{
    vector<string> allfiles;

    ArgMap args = {
        {   "as",       { UpdateMode::Set, "as" }    },
        {   "asflags",  { UpdateMode::Append, "" }      },
        {   "cc",       { UpdateMode::Set, "cc"}    },
        {   "cflags",   { UpdateMode::Append, ""}      },
        {   "cxx",      { UpdateMode::Set, "c++"}   },
        {   "cxxflags", { UpdateMode::Append,""}      },
        {   "flags",    { UpdateMode::Append, ""}      },
        {   "ld",       { UpdateMode::Set, "cc"}    },
        {   "ldflags",  { UpdateMode::Append, ""}      },
        {   "ldfirst",  { UpdateMode::Set, ""}      },
        {   "jobs",     { UpdateMode::Set, "0"}     },
        {   "workdir",  { UpdateMode::Set, ""}      },
        {   "out",      { UpdateMode::Set, "b.out"}      },
        // flags
        {   "thread",   { UpdateMode::Set, "0"}     },
        {   "shared",   { UpdateMode::Set, "0"}     },
        {   "strict",   { UpdateMode::Set, "0"}     },
        {   "release",  { UpdateMode::Set, "0"}     },
        {   "debug",    { UpdateMode::Set, "0"}     },
        {   "pipe",     { UpdateMode::Set, "1"}     },
        {   "nolink",   { UpdateMode::Set, "0"}     },
        {   "verbose",  { UpdateMode::Set, "0"}     },
        {   "clean",    { UpdateMode::Set, "0"}     },
        // c flags
        {   "c89",      { UpdateMode::Set, "0"}     },
        {   "c99",      { UpdateMode::Set, "1"}     },
        {   "c11",      { UpdateMode::Set, "0"}     },
        // c++ flags
        {   "c++11",    { UpdateMode::Set, "0"}     },
        {   "c++14",    { UpdateMode::Set, "1"}     },
        {   "c++1y",    { UpdateMode::Set, "0"}     },
        {   "c++1z",    { UpdateMode::Set, "0"}     }
    };

    auto UpdateValue = [&](const std::string& key, const std::string& value = "") {
        auto iter = args.find(key);
        if (iter == args.end()) {
            return false;
        }
        switch (iter->second.first) {
            case UpdateMode::Set: {
                iter->second.second = value.empty() ? "1" : value;
            } break;
            case UpdateMode::Append: {
                if (iter->second.second.empty()) {
                    iter->second.second = value;
                } else {
                    iter->second.second += " " + value;
                }
            } break;
        }
        return true;
    };

    // parse arguments, file name should not contain '='
    {
        auto ok = ParseArguments(argc, argv, [&](const std::string& key, const std::string& value = "") {
            auto hit = UpdateValue(key, value);
            if (hit) {
                return true;
            }
            if (key == "help") {
                Usage(argv[0]);
                exit(EXIT_SUCCESS);
            } else if (key == "prefix") {
                args["flags"].second += " -I " + value + "/include";
                args["ldflags"].second += " -L " + value + "/lib";
                return true;
            }
            switch (FileInfo(key).Type()) {
                case FileType::Directory: {
                    auto files = Dir::ListDir(key);
                    std::transform(files.begin(), files.end(), files.begin(), [&](const std::string& file) {
                        return key + "/" + file;
                    });
                    allfiles.reserve(allfiles.size() + files.size());
                    allfiles.insert(allfiles.end(), files.begin(), files.end());
                    return true;
                };
                case FileType::Regular: {
                    allfiles.push_back(key);
                    return true;
                };
                default: break;
            }
            cerr << "Bad argument: " << key << "=" << value << endl;
            return true;
        });

        if (!ok) {
            return Error::Argument;
        }

        if (!args["workdir"].second.empty()) {
            auto& dir = args["workdir"].second;
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

        if (!args["flags"].second.empty()) {
            args["flags"].second.insert(0, 1, ' ');
        }

        if (args["strict"].second == "1") {
            args["flags"].second += " -Wall -Wextra -Werror";
        }

        if (args["release"].second == "1") {
            args["flags"].second += " -DNDEBUG";
        } else if (args["debug"].second == "1") {
            args["flags"].second += " -g";
        }

        if (args["pipe"].second == "1") {
            args["flags"].second += " -pipe";
        }

        if (args["shared"].second == "1") {
            args["flags"].second += " -fPIC";
            args["ldflags"].second += " -shared";
        }

        //-------------------------------------------------------------
        // additional link flags

        if (!args["ldflags"].second.empty()) {
            args["ldflags"].second.insert(0, 1, ' ');
        }

        if (args["thread"].second == "1") {
            args["flags"].second += " -pthread";
#ifndef __APPLE__
            args["ldflags"].second += " -pthread";
#endif
        }
        
        //-------------------------------------------------------------
        // additional c/c++ compiler flags

        if (args["cflags"].second.empty()) {
            args["cflags"].second = args["flags"].second;
        } else {
            args["cflags"].second.insert(0, args["flags"].second + " ");
        }

        if (args["c89"].second == "1") {
            args["cflags"].second += " -std=c89";
        } else if (args["c99"].second == "1") {
            args["cflags"].second += " -std=c99";
        } else if (args["c11"].second == "1") {
            args["cflags"].second += " -std=c11";
        }

        if (args["cxxflags"].second.empty()) {
            args["cxxflags"].second = args["flags"].second;
        } else {
            args["cxxflags"].second.insert(0, args["flags"].second + " ");
        }

        if (args["c++11"].second == "1") {
            args["cxxflags"].second += " -std=c++11";
        } else if (args["c++14"].second == "1") {
            args["cxxflags"].second += " -std=c++14";
        } else if (args["c++1y"].second == "1") {
            args["cxxflags"].second += " -std=c++1y";
        } else if (args["c++1z"].second == "1") {
            args["cxxflags"].second += " -std=c++1z";
        }

        //-------------------------------------------------------------

        // if unspecified, take all files under current folder
        if (allfiles.empty()) {
            allfiles = Dir::ListDir(".");
        }

        if (allfiles.empty()) {
            cerr << "FATAL: nothing to build!" << endl;
            return Error::Empty;
        }

        // sort by file path
        std::sort(allfiles.begin(), allfiles.end(), [](const std::string& str1, const std::string& str2) {
            return std::strcoll(str1.c_str(), str2.c_str()) < 0 ? true : false;
        });
    }
    
    // Prepare construct units.
    // Note: "workdir" & "out" should occur together
    bool hasCpp = false;
    bool hasOut = FileInfo(args["workdir"].second + args["out"].second).Exists();
    vector<TransUnit> newUnits;
    string allObjects;

    for (const auto& file: allfiles) {
        const auto& outdir = args["workdir"].second;
        bool isCpp = false;
        auto unit = TransUnit::Make(file, outdir, args, isCpp);
        if (!unit) {
            continue;
        }
        hasCpp = hasCpp || isCpp;
        if (unit.objfile == args["ldfirst"].second) {
            allObjects = " " + unit.objfile + allObjects;
        } else {
            allObjects += " " + unit.objfile;
        }
        if (!unit.command.empty()) {
            newUnits.push_back(std::move(unit));
        }
    }

    if (hasCpp) {
        args["ld"].second = args["cxx"].second;
    }

#ifdef DEBUG
    // Debug info.
    {
        std::string split(80, '-');
        cout << "New translation units: " << endl;
        cout << split << endl;
        for (const auto& unit: newUnits) {
            cout << unit.Note(true) << endl;
            for (const auto& dep: unit.deps) {
                cout << dep << ", ";
            }
            cout << endl << split << endl;
        }
    }
#endif

    // clean
    if (args["clean"].second == "1") {
        const string& cmd = "rm -f " + args["workdir"].second + args["out"].second + allObjects;
        cout << cmd << endl;
        ::system(cmd.c_str());
        return EXIT_SUCCESS;
    }

    auto verbose = args["verbose"].second == "1";
    auto nolink = args["nolink"].second == "1";

    // compile
    if (!newUnits.empty()) {
        cout << "* Build: ";
        if (verbose) {
            for (size_t i = 0; i < newUnits.size(); ++i) {
                cout << newUnits[i].srcfile << ((i+1 < newUnits.size()) ? ", " : "");
            }
        } else {
            cout << newUnits.size() << (newUnits.size() > 1 ? " files" : " file");
        }
        cout << endl;

        if (Compiler::Run(newUnits, std::stoi(args["jobs"].second), verbose) != 0) {
            return Error::Compile;
        }
    }

    // link
    if ((!hasOut || !newUnits.empty()) && !nolink) {
        string ldCmd = args["ld"].second + args["ldflags"].second;
        ldCmd += " -o " + args["workdir"].second + args["out"].second + allObjects;

        if (verbose) {
            cout << "- Link - " << ldCmd << endl;
        } else {
            cout << "- Link - " << args["workdir"].second + args["out"].second << endl;
        }
        if (::system(ldCmd.c_str()) != 0) {
            cerr << "FATAL: failed to link!" << endl;
            cerr << "    file:   " << allObjects << endl;
            cerr << "    ld:     " << args["ld"].second << endl;
            cerr << "    ldflags: " << args["ldflags"].second << endl;
            return Error::Compile;
        }
    }

    return EXIT_SUCCESS;
}
