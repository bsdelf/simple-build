#include "ConsUnit.h"
#include "Tools.h"

#include <iostream>

#include "scx/FileInfo.hpp"
using namespace scx;

// Maybe we can cache mtime to reduce the stat() system calls.
bool ConsUnit::Init(ConsUnit& u, const string& c, const string& f)
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
            u.cmd = c + f + " -c " + u.in + " -o " + u.out;
#ifdef DEBUG
            u.deps.assign(l.begin()+1, l.end());
#endif
        }
        return true;
    } else {
        return false;
    }
}
