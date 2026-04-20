#include <gtest/gtest.h>
#include <sstream>
#include <iostream>

#include "irbuilder.hh"

namespace {
    std::string getDump(IRBuilder& builder) {
        std::stringstream ss;
        std::streambuf* old_cout = std::cout.rdbuf();
        std::cout.rdbuf(ss.rdbuf());
        builder.dump();
        std::cout.rdbuf(old_cout);
        return ss.str();
    }

    int countSubstrings(const std::string& str, const std::string& sub) {
        int count = 0;
        size_t pos = 0;
        while ((pos = str.find(sub, pos)) != std::string::npos) {
            count++;
            pos += sub.length();
        }
        return count;
    }
}

TEST(CheckOptimization, SimpleDominance) {
    IRBuilder builder;
    auto* entry = builder.createBasicBlock("entry");
    auto p = builder.createAlloca();
    builder.createNullCheck(p);
    builder.createBr("next");

    builder.createBasicBlock("next");
    builder.createNullCheck(p);
    builder.createRet();

    builder.buildCFG();
    
    std::string before = getDump(builder);
    EXPECT_EQ(2, countSubstrings(before, "check.null"));

    builder.optimizeChecks();
    
    std::string after = getDump(builder);
    EXPECT_EQ(1, countSubstrings(after, "check.null"));
}

TEST(CheckOptimization, IntraBlock) {
    IRBuilder builder;
    auto* entry = builder.createBasicBlock("entry");
    auto p = builder.createAlloca();
    builder.createNullCheck(p);
    builder.createNullCheck(p);
    builder.createRet();

    builder.buildCFG();

    std::string before = getDump(builder);
    EXPECT_EQ(2, countSubstrings(before, "check.null"));

    builder.optimizeChecks();
    
    std::string after = getDump(builder);
    EXPECT_EQ(1, countSubstrings(after, "check.null"));
}

TEST(CheckOptimization, NoDominance) {
    IRBuilder builder;
    auto* entry = builder.createBasicBlock("entry");
    auto p = builder.createAlloca();
    auto cmp = builder.createICmp(ICmpInst::EQ, 1, 0);
    builder.createBr(cmp, "then", "else");

    builder.createBasicBlock("then");
    builder.createNullCheck(p);
    builder.createBr("merge");

    builder.createBasicBlock("else");
    builder.createNullCheck(p);
    builder.createBr("merge");

    builder.createBasicBlock("merge");
    builder.createRet();
    
    builder.buildCFG();
    
    std::string before = getDump(builder);
    EXPECT_EQ(2, countSubstrings(before, "check.null"));

    builder.optimizeChecks();
    
    std::string after = getDump(builder);
    EXPECT_EQ(2, countSubstrings(after, "check.null"));
}


TEST(CheckOptimization, BoundsCheckSameIndex) {
    IRBuilder builder;
    auto* entry = builder.createBasicBlock("entry");
    auto p = builder.createAlloca();
    auto idx = builder.createAdd(0, 1);
    builder.createBoundsCheck(p, idx);
    builder.createBr("next");

    builder.createBasicBlock("next");
    builder.createBoundsCheck(p, idx);
    builder.createRet();

    builder.buildCFG();
    
    std::string before = getDump(builder);
    EXPECT_EQ(2, countSubstrings(before, "check.bounds"));

    builder.optimizeChecks();
    
    std::string after = getDump(builder);
    EXPECT_EQ(1, countSubstrings(after, "check.bounds"));
}

TEST(CheckOptimization, BoundsCheckConstantIndex) {
    IRBuilder builder;
    auto* entry = builder.createBasicBlock("entry");
    auto p = builder.createAlloca();
    builder.createBoundsCheck(p, Operand(5));
    builder.createBr("next");

    builder.createBasicBlock("next");
    builder.createBoundsCheck(p, Operand(5));
    builder.createRet();

    builder.buildCFG();
    
    std::string before = getDump(builder);
    EXPECT_EQ(2, countSubstrings(before, "check.bounds"));

    builder.optimizeChecks();
    
    std::string after = getDump(builder);
    EXPECT_EQ(1, countSubstrings(after, "check.bounds"));
    EXPECT_EQ(1, countSubstrings(after, "check.bounds %v0, 5"));
}

TEST(CheckOptimization, BoundsCheckDifferentConstantIndex) {
    IRBuilder builder;
    auto* entry = builder.createBasicBlock("entry");
    auto p = builder.createAlloca();
    builder.createBoundsCheck(p, Operand(5));
    builder.createBr("next");

    builder.createBasicBlock("next");
    builder.createBoundsCheck(p, Operand(10));
    builder.createRet();

    builder.buildCFG();
    
    std::string before = getDump(builder);
    EXPECT_EQ(2, countSubstrings(before, "check.bounds"));

    builder.optimizeChecks();
    
    std::string after = getDump(builder);
    // Expect both checks to remain since they are for different indices
    EXPECT_EQ(2, countSubstrings(after, "check.bounds"));
    EXPECT_EQ(1, countSubstrings(after, "check.bounds %v0, 5"));
    EXPECT_EQ(1, countSubstrings(after, "check.bounds %v0, 10"));
}