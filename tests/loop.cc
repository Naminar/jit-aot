
#include "loop.hh"

void verifyLoops(const std::vector<Loop*>& actualLoops,
                 std::vector<ExpectedLoopInfo>& expectedLoops) {
    
    ASSERT_EQ(actualLoops.size(), expectedLoops.size()) << "Incorrect number of loops found.";


    std::vector<const Loop*> sortedActual(actualLoops.begin(), actualLoops.end());

    auto sortRule = [](const Loop* a, const Loop* b) {
        return a->header->name < b->header->name;
    };
    std::sort(sortedActual.begin(), sortedActual.end(), sortRule);

    auto expectedSortRule = [](const ExpectedLoopInfo& a, const ExpectedLoopInfo& b) {
        return a.header < b.header;
    };
    std::sort(expectedLoops.begin(), expectedLoops.end(), expectedSortRule);

    for (size_t i = 0; i < sortedActual.size(); ++i) {
        const auto* actual = sortedActual[i];
        const auto& expected = expectedLoops[i];

        ASSERT_NE(actual->header, nullptr);
        std::string headerName = actual->header->name;
        ASSERT_EQ(headerName, expected.header) << "Loop at index " << i << " has wrong header.";

        std::unordered_set<std::string> actualBackEdges;
        for (const auto* be : actual->backEdges) {
            actualBackEdges.insert(be->name);
        }
        EXPECT_EQ(actualBackEdges, expected.backEdges) << "Mismatch in back edges for loop with header " << headerName;

        std::unordered_set<std::string> actualBlocks;
        for (const auto* block : actual->blocks) {
            actualBlocks.insert(block->name);
        }
        EXPECT_EQ(actualBlocks, expected.blocks) << "Mismatch in blocks for loop with header " << headerName;
    }
}