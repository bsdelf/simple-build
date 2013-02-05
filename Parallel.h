#ifndef PARALLEL_H
#define PARALLEL_H

#include <mutex>
#include <vector>
using namespace std;

#include "ConsUnit.h"

class ParallelCompiler
{
public:
    ParallelCompiler(const vector<ConsUnit>& units, size_t buildCount);

    int Run(int jobs = 0);

    string Objects() const;

private:
    int Worker();

private:
    const vector<ConsUnit>& m_Units;
    const size_t m_BuildCount;

    size_t m_UnitIndex = 0;
    size_t m_BuildIndex = 0;
    mutex m_IndexMutex;

    string m_Objects;
    mutex m_ObjectsMutex;

    mutex m_CoutMutex;

    bool m_Ok = true;
};


#endif
