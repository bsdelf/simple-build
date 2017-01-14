#include "Parallel.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <future>
#include <thread>

int ParallelCompiler::Run(int jobs)
{
    if (jobs <= 0) {
        jobs = std::thread::hardware_concurrency();
    }

    auto doWork = [this]() {
        while (m_Ok) {
            // Take unit.
            int uidx = 0;
            {
                std::unique_lock<std::mutex> locker(m_IndexMutex);
                if (m_UnitIndex >= m_Units.size()) {
                    return 0;
                }
                uidx = m_UnitIndex++;
            }

            // Show progress.
            const auto& unit = m_Units[uidx];
            int percent = (double)(uidx+1) / m_Units.size() * 100;
            {
                std::unique_lock<std::mutex> locker(m_CoutMutex);
                if (m_Verbose) {
                    ::printf("[ %3d%% ] %s\n", percent, unit.cmd.c_str());
                } else {
                    ::printf("[ %3d%% ] %s => %s\n", percent, unit.in.c_str(), unit.out.c_str());
                }
            }

            // Try compile it.
            if (::system(unit.cmd.c_str()) != 0) {
                m_Ok = false;
                return -1;
            }
        }
        return 0;
    };

    std::vector<std::future<int>> workers(jobs);

    for (auto& worker: workers) {
        worker = std::async(std::launch::async, doWork);
    }

    for (auto& worker: workers) {
        int ret = worker.get();
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}
