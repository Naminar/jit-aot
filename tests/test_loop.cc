#include "loop.hh"

TEST(LoopAnalysis, Loop1) {
    IRBuilder builder;
    builder.createBasicBlock("entry");
    builder.createBr("B");

    builder.createBasicBlock("B");
    builder.createBr(" ", "C", "D");

    builder.createBasicBlock("C");
    builder.createRet(" ");

    builder.createBasicBlock("D");
    builder.createBr("E");

    builder.createBasicBlock("E");
    builder.createBr("B");

    builder.dump();

    builder.printDominators();

    builder.analyzeLoops();

    auto actualLoops = builder.loopList;
    
    std::vector<ExpectedLoopInfo> expectedLoops = {
        {"B", {"E"}, {"D", "E", "B"}}
    };
    
    verifyLoops(actualLoops, expectedLoops);
}

TEST(LoopAnalysis, Loop2) {
    IRBuilder builder;
    builder.createBasicBlock("entry");
    builder.createBr("B");

    builder.createBasicBlock("B");
    builder.createBr("C");

    builder.createBasicBlock("C");
    builder.createBr(" ", "D", "E");

    builder.createBasicBlock("D");
    builder.createRet(" ");

    builder.createBasicBlock("E");
    builder.createBr(" ", "D", "F");

    builder.createBasicBlock("F");
    builder.createBr("B");

    builder.dump();

    builder.printDominators();

    builder.analyzeLoops();

    auto actualLoops = builder.loopList;
    
    std::vector<ExpectedLoopInfo> expectedLoops = {
        {"B", {"F"}, {"C", "E", "F", "B"}}
    };
    
    verifyLoops(actualLoops, expectedLoops);
}

TEST(LoopAnalysis, Loop3) {
    IRBuilder builder;
    builder.createBasicBlock("entry");
    builder.createBr("B");

    builder.createBasicBlock("B");
    builder.createBr(" ", "C", "G");

    builder.createBasicBlock("G");
    builder.createBr(" ", "D", "I");

    builder.createBasicBlock("I");
    builder.createRet(" ");

    builder.createBasicBlock("C");
    builder.createBr("D");

    builder.createBasicBlock("D");
    builder.createBr("E");

    builder.createBasicBlock("E");
    builder.createBr(" ", "B", "F");

    builder.createBasicBlock("F");
    builder.createBr("entry");

    builder.dump();

    builder.printDominators();

    builder.analyzeLoops();

    auto actualLoops = builder.loopList;
    
    std::vector<ExpectedLoopInfo> expectedLoops = {
        {"B", {"E"}, {"G", "C", "D", "E", "B"}},
        {"entry", {"F"}, {"F", "entry"}}
    };
    
    verifyLoops(actualLoops, expectedLoops);
}