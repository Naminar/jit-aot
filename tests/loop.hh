#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#include "irbuilder.hh"

struct ExpectedLoopInfo {
    std::string header;
    std::unordered_set<std::string> backEdges;
    std::unordered_set<std::string> blocks;
};

void verifyLoops(const std::vector<Loop*>& actualLoops,
                 std::vector<ExpectedLoopInfo>& expectedLoops);