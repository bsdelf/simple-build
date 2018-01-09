#pragma once

#include <mutex>
#include <vector>

#include "ConsUnit.h"

class Compiler
{
public:
    static int Run(const std::vector<ConsUnit>& units, int jobs, bool verbose);
};
