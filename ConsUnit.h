#ifndef CONSUNIT_H
#define CONSUNIT_H

#include <string>
#include <vector>
using namespace std;

struct ConsUnit
{
    string dir; // output dir
    string in;

    string out;
    string cmd;

#ifdef DEBUG
    vector<string> deps;
#endif

    ConsUnit(const string& workdir, const string& infile): dir(workdir), in(infile) { }

    static bool InitC(ConsUnit& unit, const string& compiler, const string& flag);
    static bool InitCpp(ConsUnit& unit, const string& compiler, const string& flag);
    static bool InitAsm(ConsUnit& unit, const string& compiler, const string& flag);
};

#endif
