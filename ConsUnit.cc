#include "ConsUnit.h"
#include "Tools.h"

#include <iostream>

#include "scx/FileInfo.hpp"
using namespace scx;

// Maybe we can cache mtime to reduce the stat() system calls.
bool ConsUnit::InitC(ConsUnit& u, const string& c, const string& f)
{
    const string& cmd(c + " -MM " + u.in + " " + f);
    const string& deps(DoCmd(cmd));
    const string& re("(\\s)+(\\\\)*(\\s)*");
    
    const auto& l = RegexSplit(deps, re);
    if (l.size() >= 2) {
        u.out = FileInfo(u.in).BaseName() + ".o";//l[0].substr(0, l[0].size()-1);
        u.out = u.dir + u.out;

        bool need = false;
        if (FileInfo(u.out).Exists()) {
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
            u.cmd = c + f + " -o " + u.out + " -c " + u.in;
#ifdef DEBUG
            u.deps.assign(l.begin()+1, l.end());
#endif
        }
        return true;
    } else {
        return false;
    }
}

bool ConsUnit::InitCpp(ConsUnit& unit, const string& compiler, const string& flag)
{
    return InitC(unit, compiler, flag);
}

bool ConsUnit::InitAsm(ConsUnit& unit, const string& compiler, const string& flag)
{
    unit.out = FileInfo(unit.in).BaseName() + ".o";

    if (!FileInfo(unit.out).Exists() || IsNewer(unit.in, unit.out)) {
        unit.cmd = compiler + " " + flag + " -o " + unit.out + " " + unit.in;
    }

    return true;
}


