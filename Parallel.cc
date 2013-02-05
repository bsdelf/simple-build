#include "Parallel.h"

#include <assert.h>

#include <future>
#include <thread>
#include <iostream>

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
        worker = async( bind(&ParallelCompiler::Worker, ref(*this)) );
    }

    for (auto& worker: workers) {
        int ret = 0;
        if ( ( ret = worker.get() ) != 0)
            return ret;
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
        const ConsUnit& unit = m_Units[uidx];
        char buf[] = "[ 100% ] ";
        int percent = (double)(uidx+1) / m_Units.size() * 100;
        ::snprintf(buf, sizeof buf, "[ %3.d%% ] ", percent);

        m_CoutMutex.lock();
        cout << buf << unit.cmd << endl;
        m_CoutMutex.unlock();

        if (::system( unit.cmd.c_str() ) != 0) {
            m_Ok = false;
            return -1;
        }
    }
    return 0;
}
