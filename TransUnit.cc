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
TransUnit MakeForC(std::string in, std::string out, std::string compiler, std::string flags)
{
    auto deps = RegexSplit(DoCmd(compiler + " -MM " + in + " " + flags), "(\\s)+(\\\\)*(\\s)*");
    if (deps.size() < 2) {
        return {};
    }

    const bool required = FileInfo(out).Exists() ?
        std::any_of(std::begin(deps), std::end(deps), [&](const auto& dep) { return IsNewer(dep, out); }) :
        true;

    std::string cmd;
    if (required) {
        cmd = compiler + flags + " -o " + out + " -c " + in;
    }
#if !defined(DEBUG)
    return { in, out, cmd };
#else
    return { in, out, cmd, deps };
#endif
}

TransUnit MakeForAsm(std::string in, std::string out, std::string compiler, std::string flags)
{
    std::string cmd;
    if (!FileInfo(out).Exists() || IsNewer(in, out)) {
        cmd = compiler + " " + flags + " -o " + out + " " + in;
    }
#if !defined(DEBUG)
    return { in, out, cmd };
#else
    return { in, out, cmd, {} };
#endif
}

auto TransUnit::Make(std::string file, std::string outdir, const std::unordered_map<std::string, std::string>& args, bool& is_cpp) -> TransUnit {
    FileInfo file_info(file);
    if (file_info.Type() != FileType::Regular) {
        return {};
    }
    std::string out = outdir + file_info.BaseName() + ".o";
    std::string ext = file_info.Suffix();
    std::string compiler;
    std::string flags;
    if (C_EXT.find(ext) != C_EXT.end()) {
        compiler = args.at("cc");
        flags = args.at("cflags");
        is_cpp = false;
        return MakeForC(file, out, compiler, flags);
    } else if (CXX_EXT.find(ext) != CXX_EXT.end()) {
        compiler = args.at("cxx");
        flags = args.at("cxxflags");
        is_cpp = true;
        return MakeForC(file, out, compiler, flags);
    } else if (ASM_EXT.find(ext) != ASM_EXT.end()) {
        compiler = args.at("as");
        flags = args.at("asflags");
        is_cpp = false;
        return MakeForAsm(file, out, compiler, flags);
    }
    return {};
}

TransUnit::operator bool () const {
    return !in.empty() || !out.empty();
}

auto TransUnit::Note(bool verbose) const -> std::string {
    return verbose ? cmd : (in + " => " + out);
}

