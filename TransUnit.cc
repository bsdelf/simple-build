#include "TransUnit.h"
#include "Tools.h"

#include <algorithm>
#include <unordered_set>

#include "scx/FileInfo.h"
using namespace scx;

static const std::unordered_set<std::string> C_EXT = { "c" };
static const std::unordered_set<std::string> CXX_EXT = { "cc", "cxx", "cpp", "C" };
static const std::unordered_set<std::string> ASM_EXT = { "s", "S", "asm", "nas" };

// Maybe we can cache mtime to reduce the stat() system calls.
TransUnit MakeForC(const std::string& srcfile, const std::string& objfile, const std::string& compiler, const std::string& flags)
{
    auto depfiles = RegexSplit(DoCmd(compiler + " -MM " + srcfile + " " + flags), "(\\s)+(\\\\)*(\\s)*");
    if (depfiles.size() < 2) {
        return {};
    }

    const bool required = FileInfo(objfile).Exists() ?
        std::any_of(depfiles.begin(), depfiles.end(), [&](const auto& dep) { return IsNewer(dep, objfile); }) :
        true;

    std::string command;
    if (required) {
        command = compiler + flags + " -o " + objfile + " -c " + srcfile;
    }
#if !defined(DEBUG)
    return { srcfile, objfile, command };
#else
    return { srcfile, objfile, command, depfiles };
#endif
}

TransUnit MakeForAsm(const std::string& srcfile, const std::string& objfile, const std::string& compiler, const std::string& flags)
{
    std::string command;
    if (!FileInfo(objfile).Exists() || IsNewer(srcfile, objfile)) {
        command = compiler + " " + flags + " -o " + objfile + " " + srcfile;
    }
#if !defined(DEBUG)
    return { srcfile, objfile, command };
#else
    return { srcfile, objfile, command, {} };
#endif
}

auto TransUnit::Make(const std::string& file, const std::string& outdir, const ArgMap& args, bool& is_cpp) -> TransUnit {
    FileInfo fileinfo(file);
    if (fileinfo.Type() == FileType::Regular) {
        std::string objfile = outdir + fileinfo.BaseName() + ".o";
        std::string ext = fileinfo.Suffix();
        std::string compiler;
        std::string flags;
        if (C_EXT.find(ext) != C_EXT.end()) {
            compiler = args.at("cc").second;
            flags = args.at("cflags").second;
            is_cpp = false;
            return MakeForC(file, objfile, compiler, flags);
        } else if (CXX_EXT.find(ext) != CXX_EXT.end()) {
            compiler = args.at("cxx").second;
            flags = args.at("cxxflags").second;
            is_cpp = true;
            return MakeForC(file, objfile, compiler, flags);
        } else if (ASM_EXT.find(ext) != ASM_EXT.end()) {
            compiler = args.at("as").second;
            flags = args.at("asflags").second;
            is_cpp = false;
            return MakeForAsm(file, objfile, compiler, flags);
        }
    }
    return {};
}

TransUnit::operator bool () const {
    return !srcfile.empty() || !objfile.empty();
}

auto TransUnit::Note(bool verbose) const -> std::string {
    return verbose ? command : (srcfile + " => " + objfile);
}

