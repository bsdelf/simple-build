#pragma once

#include <vector>

#include "TransUnit.h"

class Compiler
{
public:
    static int Run(const std::vector<TransUnit>& units, int jobs, bool verbose);
};
