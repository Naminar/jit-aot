
#include <gtest/gtest.h>
#include "irbuilder.hh"

// int getSpillCount(IRBuilder& builder) {
//     int count = 0;
//     for (BasicBlock* bb : builder.getLinearOrder()) {
//         for (auto& instPtr : bb->instructions) {
//             if (dynamic_cast<SpillInst*>(instPtr.get())) {
//                 count++;
//             }
//         }
//     }
//     return count;
// }

TEST(RegAllocTest, ConditionalGraph) {
    IRBuilder builder;
    
    auto entry = builder.createBasicBlock("Entry");
    auto ifTrue = builder.createBasicBlock("IfTrue");
    auto ifFalse = builder.createBasicBlock("IfFalse");
    auto end = builder.createBasicBlock("End");

    builder.setInsertPoint(entry);
    auto x1 = builder.createAdd(Operand(1), Operand(2));
    auto x2 = builder.createAdd(x1, Operand(10));
    auto x3 = builder.createAdd(x2, Operand(20));
    auto x4 = builder.createAdd(x3, Operand(30)); 
    auto cond = builder.createICmp(ICmpInst::SGT, x1, Operand(5));
    builder.createBr(cond, "IfTrue", "IfFalse");

    builder.setInsertPoint(ifTrue);
    auto yt1 = builder.createAdd(x1, x2);
    auto yt2 = builder.createAdd(x3, x4);
    auto y1 = builder.createAdd(yt1, yt2);
    builder.createBr("End");

    builder.setInsertPoint(ifFalse);
    auto yf1 = builder.createSub(x1, x2);
    auto yf2 = builder.createSub(x3, x4);
    auto y2 = builder.createAdd(yf1, yf2);
    builder.createBr("End");

    builder.setInsertPoint(end);
    auto phiY = builder.createPHI();
    phiY->addIncoming(y1, "IfTrue");
    phiY->addIncoming(y2, "IfFalse");
    auto result = builder.createAdd(phiY, x4);
    builder.createRet(result);

    builder.analyzeLoops();
    builder.computeLinearOrder();
    builder.computeLiveness();

    builder.allocateRegisters(2);

    std::cout << "\n=== RegAlloc output for ConditionalGraph ===" << std::endl;
    builder.dump();
    // builder.printLinearOrder();
    // builder.printLiveness();
}

TEST(RegAllocTest, SimpleReducibleLoopGraph) {
    IRBuilder builder;

    auto entry = builder.createBasicBlock("Entry");
    auto loopHeader = builder.createBasicBlock("LoopHdr");
    auto loopBody = builder.createBasicBlock("LoopBdy");
    auto exit = builder.createBasicBlock("Exit");

    builder.setInsertPoint(entry);
    auto initI = builder.createAdd(Operand(0), Operand(0));
    auto initA = builder.createAdd(Operand(10), Operand(10));
    auto initB = builder.createAdd(Operand(20), Operand(20));
    auto initC = builder.createAdd(Operand(30), Operand(30));
    builder.createBr("LoopHdr");

    builder.setInsertPoint(loopHeader);
    auto phiI = builder.createPHI();
    auto phiA = builder.createPHI();
    auto phiB = builder.createPHI();
    auto phiC = builder.createPHI();
    
    auto cond = builder.createICmp(ICmpInst::SLT, phiI, Operand(10));
    builder.createBr(cond, "LoopBdy", "Exit");

    builder.setInsertPoint(loopBody);
    auto nextA = builder.createAdd(phiA, Operand(1));
    auto nextB = builder.createAdd(phiB, Operand(2));
    auto nextC = builder.createAdd(phiC, Operand(3));
    auto nextI = builder.createAdd(phiI, Operand(1));
    builder.createBr("LoopHdr");

    phiI->addIncoming(initI, "Entry");
    phiI->addIncoming(nextI, "LoopBdy");
    
    phiA->addIncoming(initA, "Entry");
    phiA->addIncoming(nextA, "LoopBdy");

    phiB->addIncoming(initB, "Entry");
    phiB->addIncoming(nextB, "LoopBdy");

    phiC->addIncoming(initC, "Entry");
    phiC->addIncoming(nextC, "LoopBdy");

    builder.setInsertPoint(exit);
    auto res1 = builder.createAdd(phiA, phiB);
    auto res2 = builder.createAdd(res1, phiC);
    builder.createRet(res2);

    builder.analyzeLoops();
    builder.computeLinearOrder();
    builder.computeLiveness();

    builder.allocateRegisters(2);

    std::cout << "\n=== RegAlloc output for SimpleReducibleLoopGraph ===" << std::endl;
    builder.dump();

    // builder.printLinearOrder();
    // builder.printLiveness();

}

TEST(RegAllocTest, NestedConditionInsideLoopGraph) {
    IRBuilder builder;

    auto bEntry = builder.createBasicBlock("Entry");
    auto bLoop = builder.createBasicBlock("Loop");
    auto bIf = builder.createBasicBlock("If");
    auto bThen = builder.createBasicBlock("Then");
    auto bElse = builder.createBasicBlock("Else");
    auto bLoopEnd = builder.createBasicBlock("LoopEnd");
    auto bExit = builder.createBasicBlock("Exit");

    builder.setInsertPoint(bEntry);
    auto sumA0 = builder.createAdd(Operand(0), Operand(0));
    auto sumB0 = builder.createAdd(Operand(0), Operand(0));
    auto sumC0 = builder.createAdd(Operand(0), Operand(0));
    auto i0 = builder.createAdd(Operand(0), Operand(0));
    builder.createBr("Loop");

    builder.setInsertPoint(bLoop);
    auto phiSumA1 = builder.createPHI();
    auto phiSumB1 = builder.createPHI();
    auto phiSumC1 = builder.createPHI();
    auto phiI1 = builder.createPHI();
    auto cmp = builder.createICmp(ICmpInst::SLT, phiI1, Operand(10));
    builder.createBr(cmp, "If", "Exit");

    builder.setInsertPoint(bIf);
    auto cmp2 = builder.createICmp(ICmpInst::SLT, phiI1, Operand(5));
    builder.createBr(cmp2, "Then", "Else");

    builder.setInsertPoint(bThen);
    auto sumA2 = builder.createAdd(phiSumA1, Operand(1));
    auto sumB2 = builder.createAdd(phiSumB1, Operand(2));
    auto sumC2 = builder.createAdd(phiSumC1, Operand(3));
    builder.createBr("LoopEnd");

    builder.setInsertPoint(bElse);
    auto sumA3 = builder.createSub(phiSumA1, Operand(1));
    auto sumB3 = builder.createSub(phiSumB1, Operand(2));
    auto sumC3 = builder.createSub(phiSumC1, Operand(3));
    builder.createBr("LoopEnd");

    builder.setInsertPoint(bLoopEnd);
    auto phiSumA4 = builder.createPHI();
    phiSumA4->addIncoming(sumA2, "Then");
    phiSumA4->addIncoming(sumA3, "Else");

    auto phiSumB4 = builder.createPHI();
    phiSumB4->addIncoming(sumB2, "Then");
    phiSumB4->addIncoming(sumB3, "Else");

    auto phiSumC4 = builder.createPHI();
    phiSumC4->addIncoming(sumC2, "Then");
    phiSumC4->addIncoming(sumC3, "Else");

    auto i2 = builder.createAdd(phiI1, Operand(1));
    builder.createBr("Loop");

    phiSumA1->addIncoming(sumA0, "Entry");
    phiSumA1->addIncoming(phiSumA4, "LoopEnd");

    phiSumB1->addIncoming(sumB0, "Entry");
    phiSumB1->addIncoming(phiSumB4, "LoopEnd");

    phiSumC1->addIncoming(sumC0, "Entry");
    phiSumC1->addIncoming(phiSumC4, "LoopEnd");

    phiI1->addIncoming(i0, "Entry");
    phiI1->addIncoming(i2, "LoopEnd");

    builder.setInsertPoint(bExit);
    auto res1 = builder.createAdd(phiSumA1, phiSumB1);
    auto resOut = builder.createAdd(res1, phiSumC1);
    builder.createRet(resOut);

    builder.analyzeLoops();
    builder.computeLinearOrder();
    builder.computeLiveness();

    builder.allocateRegisters(3);

    std::cout << "\n=== RegAlloc output for NestedConditionInsideLoopGraph ===" << std::endl;
    builder.dump();

    // builder.printLinearOrder();
    // builder.printLiveness();
}