#ifndef CONSUNIT_H
#define CONSUNIT_H

#include <string>
#include <vector>
using namespace std;

struct ConsUnit
{
    string in;

    string out;
    string build;

#ifdef DEBUG
    vector<string> deps;
#endif

    explicit ConsUnit(const string& infile): in(infile) { }

    static bool Init(ConsUnit& unit, const string& compiler, const string& flag);
};

#endif
