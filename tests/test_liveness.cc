#include <gtest/gtest.h>
#include <vector>
#include <utility>

#include "irbuilder.hh"

void verifyLiveRanges(Instruction* inst, IRBuilder& builder, const std::vector<std::pair<int, int>>& expected) {
    auto actual = builder.getLiveRanges(inst);
    ASSERT_EQ(actual.size(), expected.size()) << "Range count mismatch for " << inst->name;
    for (size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i].start, expected[i].first) << "Start mismatch at index " << i << " for " << inst->name;
        EXPECT_EQ(actual[i].end, expected[i].second) << "End mismatch at index " << i << " for " << inst->name;
    }
}

TEST(LivenessTest, ConditionalGraph) {
    IRBuilder builder;
    
    auto entry = builder.createBasicBlock("Entry");
    auto ifTrue = builder.createBasicBlock("IfTrue");
    auto ifFalse = builder.createBasicBlock("IfFalse");
    auto end = builder.createBasicBlock("End");

    builder.setInsertPoint(entry);
    auto x = builder.createAdd(Operand(1), Operand(2));          // %v0
    auto cond = builder.createICmp(ICmpInst::SGT, x, Operand(5)); // %v1
    builder.createBr(cond, "IfTrue", "IfFalse");

    builder.setInsertPoint(ifTrue);
    auto y1 = builder.createAdd(x, Operand(10));                  // %v2
    builder.createBr("End");

    builder.setInsertPoint(ifFalse);
    auto y2 = builder.createSub(x, Operand(10));                  // %v3
    builder.createBr("End");

    builder.setInsertPoint(end);
    auto phiY = builder.createPHI();                              // %v4
    phiY->addIncoming(y1, "IfTrue");
    phiY->addIncoming(y2, "IfFalse");
    auto result = builder.createAdd(phiY, Operand(1));            // %v5
    builder.createRet(result);

    // Run passes
    builder.analyzeLoops();
    builder.computeLinearOrder();
    builder.computeLiveness();

    // Verify Intervals
    verifyLiveRanges(x, builder, {{2, 10}, {14, 16}});
    verifyLiveRanges(cond, builder, {{4, 6}});
    verifyLiveRanges(y1, builder, {{16, 20}});
    verifyLiveRanges(y2, builder, {{10, 14}});
    verifyLiveRanges(phiY, builder, {{20, 22}});
    verifyLiveRanges(result, builder, {{22, 24}});
}

TEST(LivenessTest, SimpleReducibleLoopGraph) {
    IRBuilder builder;

    auto entry = builder.createBasicBlock("Entry");
    auto loopHeader = builder.createBasicBlock("LoopHdr");
    auto loopBody = builder.createBasicBlock("LoopBdy");
    auto exit = builder.createBasicBlock("Exit");

    builder.setInsertPoint(entry);
    auto initVal = builder.createAdd(Operand(0), Operand(0));     // %v0
    builder.createBr("LoopHdr");

    builder.setInsertPoint(loopHeader);
    auto phiVal = builder.createPHI();                            // %v1
    auto cond = builder.createICmp(ICmpInst::SLT, phiVal, Operand(10)); // %v2
    builder.createBr(cond, "LoopBdy", "Exit");

    builder.setInsertPoint(loopBody);
    auto newVal = builder.createAdd(phiVal, Operand(1));          // %v3
    builder.createBr("LoopHdr");

    phiVal->addIncoming(initVal, "Entry");
    phiVal->addIncoming(newVal, "LoopBdy");

    builder.setInsertPoint(exit);
    builder.createRet(phiVal);

    // Run passes
    builder.analyzeLoops();
    builder.computeLinearOrder();
    builder.computeLiveness();

    // Verify Intervals
    verifyLiveRanges(initVal, builder, {{2, 6}});
    verifyLiveRanges(phiVal, builder, {{6, 14}, {18, 20}});
    verifyLiveRanges(cond, builder, {{8, 10}});
    verifyLiveRanges(newVal, builder, {{14, 18}});
}

TEST(LivenessTest, NestedConditionInsideLoopGraph) {
    IRBuilder builder;

    auto bEntry = builder.createBasicBlock("Entry");
    auto bLoop = builder.createBasicBlock("Loop");
    auto bIf = builder.createBasicBlock("If");
    auto bThen = builder.createBasicBlock("Then");
    auto bElse = builder.createBasicBlock("Else");
    auto bLoopEnd = builder.createBasicBlock("LoopEnd");
    auto bExit = builder.createBasicBlock("Exit");

    builder.setInsertPoint(bEntry);
    auto sum0 = builder.createAdd(Operand(0), Operand(0));        // %v0
    auto i0 = builder.createAdd(Operand(0), Operand(0));          // %v1
    builder.createBr("Loop");

    builder.setInsertPoint(bLoop);
    auto phiSum1 = builder.createPHI();                           // %v2
    auto phiI1 = builder.createPHI();                             // %v3
    auto cmp = builder.createICmp(ICmpInst::SLT, phiI1, Operand(10)); // %v4
    builder.createBr(cmp, "If", "Exit");

    builder.setInsertPoint(bIf);
    auto cmp2 = builder.createICmp(ICmpInst::SLT, phiI1, Operand(5)); // %v5
    builder.createBr(cmp2, "Then", "Else");

    builder.setInsertPoint(bThen);
    auto sum2 = builder.createAdd(phiSum1, Operand(1));           // %v6
    builder.createBr("LoopEnd");

    builder.setInsertPoint(bElse);
    builder.createBr("LoopEnd");

    builder.setInsertPoint(bLoopEnd);
    auto phiSum3 = builder.createPHI();                           // %v7
    phiSum3->addIncoming(sum2, "Then");
    phiSum3->addIncoming(phiSum1, "Else");
    auto i2 = builder.createAdd(phiI1, Operand(1));               // %v8
    builder.createBr("Loop");

    // Resolve Loop-level PHIs
    phiSum1->addIncoming(sum0, "Entry");
    phiSum1->addIncoming(phiSum3, "LoopEnd");
    phiI1->addIncoming(i0, "Entry");
    phiI1->addIncoming(i2, "LoopEnd");

    builder.setInsertPoint(bExit);
    builder.createRet(phiSum1);

    // Run passes
    builder.analyzeLoops();
    builder.computeLinearOrder();
    builder.computeLiveness();

    // Verify Intervals
    verifyLiveRanges(sum0, builder, {{2, 8}});
    verifyLiveRanges(i0, builder, {{4, 8}});
    verifyLiveRanges(phiSum1, builder, {{8, 26}, {36, 38}});
    verifyLiveRanges(phiI1, builder, {{8, 32}});
    verifyLiveRanges(cmp, builder, {{10, 12}});
    verifyLiveRanges(cmp2, builder, {{16, 18}});
    verifyLiveRanges(sum2, builder, {{26, 30}});
    verifyLiveRanges(phiSum3, builder, {{30, 36}});
    verifyLiveRanges(i2, builder, {{32, 36}});
}