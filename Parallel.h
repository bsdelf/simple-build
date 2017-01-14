#pragma once

#include <mutex>
#include <vector>

#include "ConsUnit.h"

class ParallelCompiler
{
public:
    explicit ParallelCompiler(const std::vector<ConsUnit>& units):
        m_Units(units)
    {
    }

    void SetVerbose(bool verbose)
    {
        m_Verbose = verbose;
    }

    int Run(int jobs = 0);

private:
    const std::vector<ConsUnit>& m_Units;
    bool m_Verbose = false;

    size_t m_UnitIndex = 0;
    std::mutex m_IndexMutex;

    std::mutex m_CoutMutex;

    bool m_Ok = true;
};
