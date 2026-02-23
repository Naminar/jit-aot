
#include "irbuilder.hh"
#include <iostream>

void testConditionalGraph() {
    std::cout << "\n======================================\n";
    std::cout << " TEST 1: Conditional Control Flow Graph\n";
    std::cout << "======================================\n";
    IRBuilder builder;
    
    auto entry = builder.createBasicBlock("Entry");
    auto ifTrue = builder.createBasicBlock("IfTrue");
    auto ifFalse = builder.createBasicBlock("IfFalse");
    auto end = builder.createBasicBlock("End");

    builder.setInsertPoint(entry);
    auto x = builder.createAdd(Operand(1), Operand(2));
    auto cond = builder.createICmp(ICmpInst::SGT, x, Operand(5));
    builder.createBr(cond, "IfTrue", "IfFalse");

    builder.setInsertPoint(ifTrue);
    auto y1 = builder.createAdd(x, Operand(10));
    builder.createBr("End");

    builder.setInsertPoint(ifFalse);
    auto y2 = builder.createSub(x, Operand(10));
    builder.createBr("End");

    builder.setInsertPoint(end);
    auto phiY = builder.createPHI();
    phiY->addIncoming(y1, "IfTrue");
    phiY->addIncoming(y2, "IfFalse");
    auto result = builder.createAdd(phiY, Operand(1));
    builder.createRet(result);

    builder.analyzeLoops();
    builder.computeLinearOrder();
    builder.computeLiveness();

    builder.printLinearOrder();
    builder.printLiveness();
}

void testSimpleReducibleLoopGraph() {
    std::cout << "\n======================================\n";
    std::cout << " TEST 2: Simple Reducible Loop Graph\n";
    std::cout << "======================================\n";
    IRBuilder builder;

    auto entry = builder.createBasicBlock("Entry");
    auto loopHeader = builder.createBasicBlock("LoopHdr");
    auto loopBody = builder.createBasicBlock("LoopBdy");
    auto exit = builder.createBasicBlock("Exit");

    builder.setInsertPoint(entry);
    auto initVal = builder.createAdd(Operand(0), Operand(0));
    builder.createBr("LoopHdr");

    builder.setInsertPoint(loopHeader);
    auto phiVal = builder.createPHI(); // Add incomings later
    auto cond = builder.createICmp(ICmpInst::SLT, phiVal, Operand(10));
    builder.createBr(cond, "LoopBdy", "Exit");

    builder.setInsertPoint(loopBody);
    auto newVal = builder.createAdd(phiVal, Operand(1));
    builder.createBr("LoopHdr");

    phiVal->addIncoming(initVal, "Entry");
    phiVal->addIncoming(newVal, "LoopBdy");

    builder.setInsertPoint(exit);
    builder.createRet(phiVal);

    builder.analyzeLoops();
    builder.computeLinearOrder();
    builder.computeLiveness();

    builder.printLinearOrder();
    builder.printLiveness();
}

void testNestedConditionInsideLoopGraph() {
    std::cout << "\n======================================\n";
    std::cout << " TEST 3: Loop With Nested Condition\n";
    std::cout << "======================================\n";
    IRBuilder builder;

    auto bEntry = builder.createBasicBlock("Entry");
    auto bLoop = builder.createBasicBlock("Loop");
    auto bIf = builder.createBasicBlock("If");
    auto bThen = builder.createBasicBlock("Then");
    auto bElse = builder.createBasicBlock("Else");
    auto bLoopEnd = builder.createBasicBlock("LoopEnd");
    auto bExit = builder.createBasicBlock("Exit");

    builder.setInsertPoint(bEntry);
    auto sum0 = builder.createAdd(Operand(0), Operand(0));
    auto i0 = builder.createAdd(Operand(0), Operand(0));
    builder.createBr("Loop");

    builder.setInsertPoint(bLoop);
    auto phiSum1 = builder.createPHI();
    auto phiI1 = builder.createPHI();
    auto cmp = builder.createICmp(ICmpInst::SLT, phiI1, Operand(10));
    builder.createBr(cmp, "If", "Exit");

    builder.setInsertPoint(bIf);
    auto cmp2 = builder.createICmp(ICmpInst::SLT, phiI1, Operand(5));
    builder.createBr(cmp2, "Then", "Else");

    builder.setInsertPoint(bThen);
    auto sum2 = builder.createAdd(phiSum1, Operand(1));
    builder.createBr("LoopEnd");

    builder.setInsertPoint(bElse);
    builder.createBr("LoopEnd");

    builder.setInsertPoint(bLoopEnd);
    auto phiSum3 = builder.createPHI();
    phiSum3->addIncoming(sum2, "Then");
    phiSum3->addIncoming(phiSum1, "Else");
    auto i2 = builder.createAdd(phiI1, Operand(1));
    builder.createBr("Loop");

    // Resolve Loop-level PHIs
    phiSum1->addIncoming(sum0, "Entry");
    phiSum1->addIncoming(phiSum3, "LoopEnd");
    phiI1->addIncoming(i0, "Entry");
    phiI1->addIncoming(i2, "LoopEnd");

    builder.setInsertPoint(bExit);
    builder.createRet(phiSum1);

    builder.analyzeLoops();
    builder.computeLinearOrder();
    builder.computeLiveness();

    builder.printLinearOrder();
    builder.printLiveness();
}

int main() {
    testConditionalGraph();
    testSimpleReducibleLoopGraph();
    testNestedConditionInsideLoopGraph();
    return 0;
}

// int main() {
//     IRBuilder builder;

//     auto* entry = builder.createBasicBlock("entry");
//     auto x = builder.createAlloca();
//     builder.createStore("42", x);
//     auto val = builder.createLoad(x);
//     auto cmp = builder.createICmp(ICmpInst::SGT, val, "0");
//     builder.createBr(cmp, "then", "else");

//     auto* thenBB = builder.createBasicBlock("then");
//     auto a = builder.createAdd(val, "1");
//     builder.createBr("merge");

//     auto* elseBB = builder.createBasicBlock("else");
//     auto b = builder.createSub(val, "1");
//     builder.createBr("merge");

//     auto* mergeBB = builder.createBasicBlock("merge");
//     auto* phi = builder.createPHI();
//     phi->addIncoming(a, "then");
//     phi->addIncoming(b, "else");
//     builder.createRet(phi->name);

//     builder.dump();

//     builder.printDominators();

//     return 0;
// }

// int main() {
//     IRBuilder builder;

//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr(" ", "C", "F");

//     builder.createBasicBlock("C");
//     builder.createBr("D");

//     builder.createBasicBlock("D");
//     builder.createRet(" ");

//     builder.createBasicBlock("E");
//     builder.createBr("D");

//     builder.createBasicBlock("F");
//     builder.createBr(" ", "E", "G");

//     builder.createBasicBlock("G");
//     builder.createBr("D");

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }


// int main() {
//     IRBuilder builder;
//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr(" ", "C", "J");

//     builder.createBasicBlock("C");
//     builder.createBr("D");

//     builder.createBasicBlock("D");
//     builder.createBr(" ", "C", "E");

//     builder.createBasicBlock("E");
//     builder.createBr("F");

//     builder.createBasicBlock("F");
//     builder.createBr(" ", "E", "G");

//     builder.createBasicBlock("J");
//     builder.createBr("C");

//     builder.createBasicBlock("G");
//     builder.createBr(" ", "H", "I");

//     builder.createBasicBlock("H");
//     builder.createBr("B");

//     builder.createBasicBlock("I");
//     builder.createBr("K");

//     builder.createBasicBlock("K");
//     builder.createRet(" ");

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }


// int main() {
//     IRBuilder builder;
//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr(1, "E", "C");

//     builder.createBasicBlock("C");
//     builder.createBr("D");

//     builder.createBasicBlock("D");
//     builder.createBr("G");

//     builder.createBasicBlock("E");
//     builder.createBr(1, "F", "D");

//     builder.createBasicBlock("F");
//     builder.createBr(1, "B", "H");

//     builder.createBasicBlock("G");
//     builder.createBr(1, "C", "I");

//     builder.createBasicBlock("H");
//     builder.createBr(1, "G", "I");

//     builder.createBasicBlock("I");
//     builder.createRet();

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }

//---------------------------- special for loop test ----------------------------------------

// int main () {
//     IRBuilder builder;
//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr(" ", "C", "D");

//     builder.createBasicBlock("C");
//     builder.createRet(" ");

//     builder.createBasicBlock("D");
//     builder.createBr("E");

//     builder.createBasicBlock("E");
//     builder.createBr("B");

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }


// int main () {
//     IRBuilder builder;
//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr("C");

//     builder.createBasicBlock("C");
//     builder.createBr(" ", "D", "E");

//     builder.createBasicBlock("D");
//     builder.createRet(" ");

//     builder.createBasicBlock("E");
//     builder.createBr(" ", "D", "F");

//     builder.createBasicBlock("F");
//     builder.createBr("B");

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }

// int main () {
//     IRBuilder builder;
//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr(" ", "C", "G");

//     builder.createBasicBlock("G");
//     builder.createBr(" ", "D", "I");

//     builder.createBasicBlock("I");
//     builder.createRet(" ");

//     builder.createBasicBlock("C");
//     builder.createBr("D");

//     builder.createBasicBlock("D");
//     builder.createBr("E");

//     builder.createBasicBlock("E");
//     builder.createBr(" ", "B", "F");

//     builder.createBasicBlock("F");
//     builder.createBr("entry");

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }

