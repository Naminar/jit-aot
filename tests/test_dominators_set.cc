#include <gtest/gtest.h>
#include <sstream>
#include <iostream>

#include "loop.hh"
// #include "irbuilder.hh"

bool compareSets(const std::unordered_set<std::string>& a,
                 const std::unordered_set<std::string>& b) {
    return a == b;
}

TEST(GeneralTest, SimpleIfElse) {
    IRBuilder builder;

    auto* entry = builder.createBasicBlock("entry");
    auto x = builder.createAlloca();
    builder.createStore(42, x);
    auto val = builder.createLoad(x);
    auto cmp = builder.createICmp(ICmpInst::SGT, val, 0);
    builder.createBr(cmp, "then", "else");

    auto* thenBB = builder.createBasicBlock("then");
    auto a = builder.createAdd(val, 1);
    builder.createBr("merge");

    auto* elseBB = builder.createBasicBlock("else");
    auto b = builder.createSub(val, 1);
    builder.createBr("merge");

    auto* mergeBB = builder.createBasicBlock("merge");
    auto* phi = builder.createPHI();
    phi->addIncoming(a, "then");
    phi->addIncoming(b, "else");
    builder.createRet(phi);

    auto dominators = builder.printDominators();

    std::unordered_map<std::string, std::unordered_set<std::string>> expected = {
        {"entry", {"entry", "then", "merge", "else"}},
        {"then", {"then"}},
        {"else", {"else"}},
        {"merge", {"merge"}}
    };

    for (auto& [name, expectedSet] : expected) {
        ASSERT_TRUE(dominators.count(name));
        EXPECT_TRUE(compareSets(dominators[name], expectedSet))
            << "Mismatch in dominators for block " << name;
    }
}

TEST(Test1, Test1) {
    IRBuilder builder;

    builder.createBasicBlock("entry");
    builder.createBr("B");

    builder.createBasicBlock("B");
    builder.createBr(1, "C", "F");

    builder.createBasicBlock("C");
    builder.createBr("D");

    builder.createBasicBlock("D");
    builder.createRet();

    builder.createBasicBlock("E");
    builder.createBr("D");

    builder.createBasicBlock("F");
    builder.createBr(1, "E", "G");

    builder.createBasicBlock("G");
    builder.createBr("D");

    auto dominators = builder.printDominators();

    builder.analyzeLoops();

    auto actualLoops = builder.loopList;
    
    std::vector<ExpectedLoopInfo> expectedLoops = {
    };
    
    verifyLoops(actualLoops, expectedLoops);

    std::unordered_map<std::string, std::unordered_set<std::string>> expected = {
        {"entry", {"entry", "B", "C", "D", "F", "E", "G"}},
        {"B", {"B", "C", "D", "F", "E", "G"}},
        {"C", {"C"}},
        {"D", {"D"}},
        {"E", {"E"}},
        {"F", {"F", "E", "G"}},
        {"G", {"G"}}
    };

    for (auto& [name, expectedSet] : expected) {
        ASSERT_TRUE(dominators.count(name));
        EXPECT_TRUE(compareSets(dominators[name], expectedSet))
            << "Mismatch in dominators for block " << name;
    }
}

TEST(Test2, Test2) {
    IRBuilder builder;

    builder.createBasicBlock("entry");
    builder.createBr("B");

    builder.createBasicBlock("B");
    builder.createBr(1, "C", "J");

    builder.createBasicBlock("C");
    builder.createBr("D");

    builder.createBasicBlock("D");
    builder.createBr(1, "C", "E");

    builder.createBasicBlock("E");
    builder.createBr("F");

    builder.createBasicBlock("F");
    builder.createBr(1, "E", "G");

    builder.createBasicBlock("J");
    builder.createBr("C");

    builder.createBasicBlock("G");
    builder.createBr(1, "H", "I");

    builder.createBasicBlock("H");
    builder.createBr("B");

    builder.createBasicBlock("I");
    builder.createBr("K");

    builder.createBasicBlock("K");
    builder.createRet();

    builder.analyzeLoops();

    auto actualLoops = builder.loopList;
    
    std::vector<ExpectedLoopInfo> expectedLoops = {
    {"E", {"F"}, {"F", "E"}}, 
    {"C", {"D"}, {"D", "C"}}, 
    {"B", {"H"}, {"J", "H", "G", "B"}}
    };
    
    verifyLoops(actualLoops, expectedLoops);

    auto dominators = builder.printDominators();
    std::unordered_map<std::string, std::unordered_set<std::string>> expected = {
        {"entry", {"entry", "I", "B", "C", "K", "D", "E", "J", "F", "G", "H"}},
        {"B", {"I", "B", "C", "K", "D", "E", "J", "F", "G", "H"}},
        {"C", {"I", "C", "K", "D", "E", "F", "G", "H"}},
        {"D", {"I", "K", "D", "E", "F", "G", "H"}},
        {"E", {"I", "K", "E", "F", "G", "H"}},
        {"F", {"I", "K", "F", "G", "H"}},
        {"J", {"J"}},
        {"G", {"I", "K", "G", "H"}},
        {"H", {"H"}},
        {"I", {"I", "K"}},
        {"K", {"K"}}
    };

    for (auto& [name, expectedSet] : expected) {
        ASSERT_TRUE(dominators.count(name));
        EXPECT_TRUE(compareSets(dominators[name], expectedSet))
            << "Mismatch in dominators for block " << name;
    }
}

TEST(Test3, Test3) {
    IRBuilder builder;

    builder.createBasicBlock("entry");
    builder.createBr("B");

    builder.createBasicBlock("B");
    builder.createBr(1, "E", "C");

    builder.createBasicBlock("C");
    builder.createBr("D");

    builder.createBasicBlock("D");
    builder.createBr("G");

    builder.createBasicBlock("E");
    builder.createBr(1, "F", "D");

    builder.createBasicBlock("F");
    builder.createBr(1, "B", "H");

    builder.createBasicBlock("G");
    builder.createBr(1, "C", "I");

    builder.createBasicBlock("H");
    builder.createBr(1, "G", "I");

    builder.createBasicBlock("I");
    builder.createRet();

    auto dominators = builder.printDominators();

    builder.analyzeLoops();

    auto actualLoops = builder.loopList;
    
    std::vector<ExpectedLoopInfo> expectedLoops = {
    {"G", {"D"}, {"C", "D", "G"}}, 
    {"B", {"F"}, {"E", "F", "B"}}
    };

    verifyLoops(actualLoops, expectedLoops);

    std::unordered_map<std::string, std::unordered_set<std::string>> expected = {
        {"entry", {"entry", "H", "B", "G", "E", "F", "C", "D", "I"}},
        {"B", {"H", "B", "G", "E", "F", "C", "D", "I"}},
        {"C", {"C"}},
        {"D", {"D"}},
        {"E", {"H", "E", "F"}},
        {"F", {"H", "F"}},
        {"G", {"G"}},
        {"H", {"H"}},
        {"I", {"I"}}
    };

    for (auto& [name, expectedSet] : expected) {
        ASSERT_TRUE(dominators.count(name));
        EXPECT_TRUE(compareSets(dominators[name], expectedSet))
            << "Mismatch in dominators for block " << name;
    }
}