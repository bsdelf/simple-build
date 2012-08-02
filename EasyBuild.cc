#include "EasyBuild.h"
#include "Tools.h"

#include <iostream>

#include "scx/FileInfo.hpp"
using namespace scx;

// Maybe we can cache mtime to reduce the stat() system calls.
bool InitConstUnit(ConsUnit& u, const string& c, const string& f)
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
            u.build = c + " -o " + u.out + " -c " + u.in + " " + f;
#ifdef DEBUG
            u.deps.assign(l.begin()+1, l.end());
#endif
        }
        return true;
    } else {
        return false;
    }
}

void Usage(const string& cmd)
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
        "\t" + sp + " clean       Clean build output.\n"
        "\t" + sp + " help        Show this help message.\n"
        "\n"
        "\t" + sp + " shared      Generate shared library. (*)\n"
        "\t" + sp + " debug       Build with debug symbols. (*)\n"
        "\t" + sp + " clang++11   Use clang & libc++. (*)\n"
        "\n";

    cout << ""
        "Note:\n"
        "\t(*) Just for convenient. You may also use flags to approach the function.\n"
        "\n";

    cout << ""
        "Author:\n"
        "\tYanhui Shen <@pagedir on Twitter>\n";
}


