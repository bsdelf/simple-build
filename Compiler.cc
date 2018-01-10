#include "Compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <algorithm>
#include <future>
#include <thread>

int Compiler::Run(const std::vector<TransUnit>& units, int jobs, bool verbose)
{
    if (jobs <= 0) {
        jobs = std::thread::hardware_concurrency();
    }

    bool ok = true;
    size_t unit_index = 0;
    std::mutex unit_index_mutex;
    std::mutex unit_count_mutex;

    auto doWork = [&]() {
        while (ok) {
            // get index
            size_t index = 0;
            {
                std::unique_lock<std::mutex> locker(unit_index_mutex);
                if (unit_index >= units.size()) {
                    return 0;
                }
                index = unit_index++;
            }
            // show progress
            const auto& unit = units[index];
            int percent = (double)(index + 1) / units.size() * 100;
            {
                std::unique_lock<std::mutex> locker(unit_count_mutex);
                ::printf("[ %3d%% ] %s\n", percent, unit.Note(verbose).c_str());
            }
            // compile it
            if (::system(unit.cmd.c_str()) != 0) {
                ok = false;
                return -1;
            }
        }
        return 0;
    };

    std::vector<std::future<int>> workers(jobs);

    std::generate(workers.begin(), workers.end(), [&]() {
        return std::async(std::launch::async, doWork);
    });

    for (auto& worker: workers) {
        int ret = worker.get();
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}
