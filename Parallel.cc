#include "Parallel.h"

#include <assert.h>
#include <unistd.h>

#include <future>
#include <iostream>

ParallelCompiler::ParallelCompiler(const vector<ConsUnit>& units, size_t buildCount):
    m_Units(units),
    m_BuildCount(buildCount)
{
}

int ParallelCompiler::Run(int jobs)
{
    if (jobs == 0)
        jobs = sysconf(_SC_NPROCESSORS_ONLN);

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

string ParallelCompiler::Objects() const
{
    return m_Objects;
}

int ParallelCompiler::Worker()
{
    while (m_Ok) {
        // Take unit.
        int uidx;
        int bidx = -1;
        {
            std::unique_lock<std::mutex> locker(m_IndexMutex);
            if (m_UnitIndex >= m_Units.size()) {
                return 0;
            } else {
                uidx = m_UnitIndex++;
                if ( !m_Units[uidx].build.empty() ) {
                    bidx = ++m_BuildIndex;
                }
            }
        }

        // Try compile it.
        const ConsUnit& unit = m_Units[uidx];
        if ( bidx != -1 ) {
            assert(m_BuildCount != 0);

            char buf[] = "[ 100% ] ";
            int percent = (double)bidx / m_BuildCount * 100;
            ::snprintf(buf, sizeof buf, "[ %3.d%% ] ", percent);
            m_CoutMutex.lock();
            cout << buf << unit.build << endl;
            m_CoutMutex.unlock();

            if (::system( unit.build.c_str() ) != 0) {
                m_Ok = false;
                return 1;
            }
        }

        // Record all our objects.
        std::lock_guard<std::mutex> locker(m_ObjectsMutex);
        m_Objects += unit.out + " ";
    }
    return 0;
}
