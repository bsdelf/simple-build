#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <utility>
using namespace std;

#include "scx/Dir.h"
#include "scx/FileInfo.h"
using namespace scx;

#include "Utils.h"
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

int main(int argc, char* argv[])
{
    std::vector<KeyValueArgs::Command> cmds = {
        { "jobs",       "set number of jobs",           KeyValueArgs::Setter(), "0" },
        { "out",        "set output binary",            KeyValueArgs::Setter(), "b.out" },
        { "workdir",    "set work directory",           KeyValueArgs::Setter(), "." },
        {},
        { "as",         "set assembler",                KeyValueArgs::Setter(), "as" },
        { "asflags",    "add assembler flags",          KeyValueArgs::Jointer(), "" },
        { "cc",         "set c compiler",               KeyValueArgs::Setter(), "cc" },
        { "cflags",     "add c compiler flags",         KeyValueArgs::Jointer(), "" },
        { "cxx",        "set c++ compiler",             KeyValueArgs::Setter(), "c++" },
        { "cxxflags",   "add c++ compiler flags",       KeyValueArgs::Jointer(), "" },
        { "flags",      "add c & c++ compiler flags",   KeyValueArgs::KeyJointer({"cflags", "cxxflags"}) },
        { "ld",         "set linker",                   KeyValueArgs::Setter(), "cc" },
        { "ldflags",    "add linker flags",             KeyValueArgs::Jointer(), "" },
        { "ldorder",    "set link order",               KeyValueArgs::Setter(), "" },
        { "prefix",     "add search directories",       KeyValueArgs::KeyValueJointer({
                {"flags", [](const std::string& str){ return "-I" + str + "/include"; }},
                {"ldflags", [](const std::string& str){ return "-L" + str + "/lib"; }}
        })},
        {},
        { "verbose",    "verbose output",               KeyValueArgs::ValueSetter("1"), "0" },
        { "clean",      "remove output files",          KeyValueArgs::ValueSetter("1"), "0" },
        { "nolink",     "do not link",                  KeyValueArgs::ValueSetter("1"), "0" },
        { "thread",     "link against pthread",         KeyValueArgs::KeyValueJointer({
                {"flags", "-pthread"},
#ifndef __APPLE__
                {"ldflags", "-pthread"}
#endif
        })},
        { "shared",     "build shared library",         KeyValueArgs::KeyValueJointer({
                {"flags", "-fPIC"}, {"ldflags", "-shared"}
        })},
        { "pipe",       "enable -pipe",                 KeyValueArgs::KeyValueJointer({{"flags", "-pipe"}}) },
        { "debug",      "enable -g",                    KeyValueArgs::KeyValueJointer({{"flags", "-g"}}) },
        { "release",    "enable -DNDEBUG",              KeyValueArgs::KeyValueJointer({{"flags", "-DNDEBUG"}}) },
        { "strict",     "enable -Wall -Wextra -Werror", KeyValueArgs::KeyValueJointer({{"flags", "-Wall -Wextra -Werror"}}) },
        { "c89",        "enable -std=c89",              KeyValueArgs::KeyValueJointer({{"cflags", "-std=c89"}}) },
        { "c99",        "enable -std=c99",              KeyValueArgs::KeyValueJointer({{"cflags", "-std=c99"}}) },
        { "c11",        "enable -std=c11",              KeyValueArgs::KeyValueJointer({{"cflags", "-std=c11"}}) },
        { "c++11",      "enable -std=c++11",            KeyValueArgs::KeyValueJointer({{"cxxflags", "-std=c++11"}}) },
        { "c++1y",      "enable -std=c++1y",            KeyValueArgs::KeyValueJointer({{"cxxflags", "-std=c++1y"}}) },
        { "c++14",      "enable -std=c++14",            KeyValueArgs::KeyValueJointer({{"cxxflags", "-std=c++14"}}) },
        { "c++1z",      "enable -std=c++1z",            KeyValueArgs::KeyValueJointer({{"cxxflags", "-std=c++1z"}}) },
        { "c++17",      "enable -std=c++17",            KeyValueArgs::KeyValueJointer({{"cxxflags", "-std=c++17"}}) },
    };

    vector<string> allfiles;
    auto args = KeyValueArgs::Parse(argc, argv, cmds, [&](std::string&& key, std::string&& value) {
        if (key == "help") {
            const size_t n = 8;
            const std::string blank(n, ' ');
            cout << "Usage:\n" + blank + std::string(argv[0]) + " [file ...] [dir ...]\n\n";
            cout << KeyValueArgs::ToString(cmds, n) << endl;
            cout << "Contact:\n" + blank + "Yanhui Shen <@bsdelf on Twitter>\n";
            exit(EXIT_SUCCESS);
        }
        if (value.empty()) {
            switch (FileInfo(key).Type()) {
                case FileType::Directory: {
                    auto files = Dir::ListDir(key);
                    std::transform(files.begin(), files.end(), files.begin(), [&](const std::string& file) {
                        return key + "/" + file;
                    });
                    allfiles.reserve(allfiles.size() + files.size());
                    allfiles.insert(allfiles.end(), files.begin(), files.end());
                    return;
                };
                case FileType::Regular: {
                    allfiles.push_back(std::move(key));
                    return;
                };
                default: break;
            }
        }
        cerr << "Bad argument: " << key << "=" << value << endl;
    });

    // prepare source files
    if (allfiles.empty()) {
        allfiles = Dir::ListDir(".");
    }
    if (allfiles.empty()) {
        cerr << "FATAL: nothing to build!" << endl;
        return Error::Empty;
    }
    std::sort(allfiles.begin(), allfiles.end(), [](const auto& a, const auto& b) {
        return std::strcoll(a.c_str(), b.c_str()) < 0 ? true : false;
    });

    // prepare work directory
    if (!args["workdir"].empty()) {
        auto& dir = args["workdir"];
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
    
    // scan translation units
    vector<TransUnit> newUnits;
    string allObjects;
    {
        bool hasCpp = false;
        const auto& ldorder = args["ldorder"];
        std::vector<std::pair<size_t, std::string>> headObjects;
        for (const auto& file: allfiles) {
            const auto& outdir = args["workdir"];
            bool isCpp = false;
            auto unit = TransUnit::Make(file, outdir, args, isCpp);
            if (!unit) {
                continue;
            }
            hasCpp = hasCpp || isCpp;
            auto pos = ldorder.empty() ? std::string::npos : ldorder.find(FileInfo::BaseName(unit.objfile));
            if (pos != std::string::npos) {
                headObjects.emplace_back(pos, unit.objfile);
            } else {
                allObjects += unit.objfile + ' ';
            }
            if (!unit.command.empty()) {
                newUnits.push_back(std::move(unit));
            }
        }
        std::sort(headObjects.begin(), headObjects.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
        });
        for (auto&& headObject: headObjects) {
            allObjects.insert(0, std::move(headObject.second) + ' ');
        }
        if (hasCpp) {
            args["ld"] = args["cxx"];
        }
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
    if (args["clean"] == "1") {
        const string& cmd = "rm -f " + args["workdir"] + args["out"] + ' ' + allObjects;
        cout << cmd << endl;
        ::system(cmd.c_str());
        return EXIT_SUCCESS;
    }

    // compile
    auto verbose = args["verbose"] == "1";
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
        if (Compiler::Run(newUnits, std::stoi(args["jobs"]), verbose) != 0) {
            return Error::Compile;
        }
    }

    // link
    bool hasOut = FileInfo(args["workdir"] + args["out"]).Exists();
    if ((!hasOut || !newUnits.empty()) && args["nolink"] != "1") {
        string ldCmd = Join({args["ld"], args["ldflags"], "-o", args["workdir"] + args["out"], allObjects});
        if (verbose) {
            cout << "- Link - " << ldCmd << endl;
        } else {
            cout << "- Link - " << args["workdir"] + args["out"] << endl;
        }
        if (::system(ldCmd.c_str()) != 0) {
            cerr << "FATAL: failed to link!" << endl;
            cerr << "    file:   " << allObjects << endl;
            cerr << "    ld:     " << args["ld"] << endl;
            cerr << "    ldflags: " << args["ldflags"] << endl;
            return Error::Compile;
        }
    }

    return EXIT_SUCCESS;
}
