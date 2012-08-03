#ifndef EASYBUILD_H
#define EASYBUILD_H

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

    explicit ConsUnit(const string& _in): in(_in) { }
};

bool InitConstUnit(ConsUnit& u, const string& c, const string& f);

void Usage(const string& cmd);

#endif
