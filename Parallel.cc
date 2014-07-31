#include "Parallel.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <future>
#include <thread>

ParallelCompiler::ParallelCompiler(const vector<ConsUnit>& units):
    m_Units(units)
{
}

int ParallelCompiler::Run(int jobs)
{
    if (jobs <= 0)
        jobs = std::thread::hardware_concurrency();

    vector<future<int>> workers(jobs);
    for (auto& worker: workers) {
        worker = std::async(bind(&ParallelCompiler::Worker, ref(*this)));
    }

    for (auto& worker: workers) {
        int ret = worker.get();
        if (ret != 0) return ret;
    }

    return 0;
}

int ParallelCompiler::Worker()
{
    while (m_Ok) {
        // Take unit.
        int uidx = 0;
        {
            std::unique_lock<std::mutex> locker(m_IndexMutex);
            if (m_UnitIndex < m_Units.size())
                uidx = m_UnitIndex++;
            else
                return 0;
        }

        // Try compile it.
        const auto& unit = m_Units[uidx];
        int percent = (double)(uidx+1) / m_Units.size() * 100;
        {
            std::unique_lock<std::mutex> locker(m_CoutMutex);
            if (m_Verbose)
                ::printf("[ %3d%% ] %s\n", percent, unit.cmd.c_str());
            else
                ::printf("[ %3d%% ] %s => %s\n", percent, unit.in.c_str(), unit.out.c_str());
        }

        if (::system(unit.cmd.c_str()) != 0) {
            m_Ok = false;
            return -1;
        }
    }
    return 0;
}
