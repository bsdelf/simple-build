#ifndef PARALLEL_H
#define PARALLEL_H

#include <mutex>
#include <vector>
using namespace std;

#include "ConsUnit.h"

class ParallelCompiler
{
public:
    explicit ParallelCompiler(const vector<ConsUnit>& units);

    int Run(int jobs = 0);
    void SetVerbose(bool verbose) { m_Verbose = verbose; }

private:
    int Work();

private:
    const vector<ConsUnit>& m_Units;
    bool m_Verbose = false;

    size_t m_UnitIndex = 0;
    mutex m_IndexMutex;

    mutex m_CoutMutex;

    bool m_Ok = true;
};


#endif
