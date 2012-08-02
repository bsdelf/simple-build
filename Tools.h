#ifndef TOOLS_H
#define TOOLS_H

#include <string>
#include <vector>

std::string DoCmd(const std::string& cmd);

std::vector<std::string> RegexSplit(const std::string& in, const std::string& sre);

bool IsNewer(const std::string& f1, const std::string& f2);

#endif
