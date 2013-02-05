#ifndef PARALLEL_H
#define PARALLEL_H

#include <mutex>
#include <vector>
using namespace std;

#include "ConsUnit.h"

class ParallelCompiler
{
public:
    ParallelCompiler(const vector<ConsUnit>& units);

    int Run(int jobs = 0);

private:
    int Worker();

private:
    const vector<ConsUnit>& m_Units;

    size_t m_UnitIndex = 0;
    mutex m_IndexMutex;

    mutex m_CoutMutex;

    bool m_Ok = true;
};


#endif
