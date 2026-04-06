
#include <gtest/gtest.h>
#include "irbuilder.hh"

TEST(InliningTest, SchemaBasedInline) {
    // ====================================================================
    // 1. Build Callee graph
    // ====================================================================
    IRBuilder callee;
    callee.functionName = "Callee";
    auto c_start = callee.createBasicBlock("block_2_start");
    auto c_bb3 = callee.createBasicBlock("block_3");
    auto c_bb4 = callee.createBasicBlock("block_4");
    auto c_bb5 = callee.createBasicBlock("block_5");

    callee.setInsertPoint(c_start);
    auto p1 = callee.createParam(); // 11. Param1 -> v13 (using p1)
    auto p2 = callee.createParam(); // 12. Param2 -> v14 (using p2)
    callee.createBr("block_3");     // connection: block 2 start -> block 3

    callee.setInsertPoint(c_bb3);
    auto v13 = callee.createAdd(Operand(p1), Operand(1));   // 13. Use2 / 19. Constant 1
    auto v14 = callee.createAdd(Operand(p2), Operand(10));  // 14. Use3 / 20. Constant 10
    auto cmp = callee.createICmp(ICmpInst::SGT, Operand(v13), Operand(v14));
    callee.createBr(Operand(cmp), "block_4", "block_5");    // connection: block 3 -> block 4, block 5

    callee.setInsertPoint(c_bb4);
    auto v16 = callee.createAdd(Operand(v13), Operand(v14)); // 15. Def3 -> v16
    callee.createRet(Operand(v16));                          // 16. Return v16

    callee.setInsertPoint(c_bb5);
    auto v18 = callee.createSub(Operand(v13), Operand(v14)); // 17. Def4 -> v18
    callee.createRet(Operand(v18));                          // 18. Return v18

    // ====================================================================
    // 2. Build Caller graph
    // ====================================================================
    IRBuilder caller;
    caller.functionName = "Caller";
    auto m_start = caller.createBasicBlock("block_0_start");
    auto m_bb1 = caller.createBasicBlock("block_1");

    caller.setInsertPoint(m_start);
    auto c1 = caller.createAdd(Operand(0), Operand(1)); // 1. Constant 1
    auto c5 = caller.createAdd(Operand(0), Operand(5)); // 2. Constant 5
    caller.createBr("block_1");                         // connection: block 0 start -> block 1

    caller.setInsertPoint(m_bb1);
    auto v3 = caller.createAdd(Operand(c1), Operand(2)); // 3. Def1 -> v3
    auto v4 = caller.createAdd(Operand(c5), Operand(2)); // 4. Def2 -> v4
    
    // 5. CallStatic v3, v4 -> v6
    auto callInst = caller.createCall("Callee", &callee, {Operand(v3), Operand(v4)}); 
    
    // 6. Use1 v6, v1, v2
    auto use1 = caller.createAdd(Operand(callInst), Operand(c1));
    caller.createRet(Operand(use1));

    std::cout << "\n=== [SchemaBased] Caller BEFORE Inlining ===\n";
    caller.dump();

    // ====================================================================
    // 3. Perform Inlining
    // ====================================================================
    caller.inlineCall(callInst);

    std::cout << "\n=== [SchemaBased] Caller AFTER Inlining ===\n";
    caller.dump();

    // ====================================================================
    // 4. Verify
    // ====================================================================
    
    BasicBlock* b0 = nullptr;
    BasicBlock* b1 = nullptr;
    BasicBlock* b3 = nullptr;
    BasicBlock* b4 = nullptr;
    BasicBlock* b5 = nullptr;
    BasicBlock* b6 = nullptr; // Equivalent to block_1_cont

    caller.analyzeLoops(); 
    caller.computeLinearOrder();
    caller.computeLiveness();
    
    for (BasicBlock* bb : caller.getLinearOrder()) {
        if (bb->name.find("block_0_start") != std::string::npos) b0 = bb;
        if (bb->name.find("block_1") != std::string::npos && bb->name.find("cont") == std::string::npos) b1 = bb;
        if (bb->name.find("block_3") != std::string::npos) b3 = bb;
        if (bb->name.find("block_4") != std::string::npos) b4 = bb;
        if (bb->name.find("block_5") != std::string::npos) b5 = bb;
        if (bb->name.find("block_1_cont") != std::string::npos) b6 = bb;
    }

    ASSERT_NE(b0, nullptr);
    ASSERT_NE(b1, nullptr);
    ASSERT_NE(b3, nullptr);
    ASSERT_NE(b4, nullptr);
    ASSERT_NE(b5, nullptr);
    ASSERT_NE(b6, nullptr);

    // Verify connections: block 0 start -> block 1
    EXPECT_EQ(b0->successors.size(), 1);
    EXPECT_EQ(b0->successors[0], b1);

    // Verify connections: block 1 -> block 3
    EXPECT_EQ(b1->successors.size(), 1);
    EXPECT_EQ(b1->successors[0], b3);

    // Verify connections: block 3 -> block 4, block 5
    EXPECT_EQ(b3->successors.size(), 2);
    bool has_b4 = (b3->successors[0] == b4 || b3->successors[1] == b4);
    bool has_b5 = (b3->successors[0] == b5 || b3->successors[1] == b5);
    EXPECT_TRUE(has_b4);
    EXPECT_TRUE(has_b5);

    // Verify connections: block 4 -> block 6
    EXPECT_EQ(b4->successors.size(), 1);
    EXPECT_EQ(b4->successors[0], b6);

    // Verify connections: block 5 -> block 6
    EXPECT_EQ(b5->successors.size(), 1);
    EXPECT_EQ(b5->successors[0], b6);
    
    // Check that block 6 starts with a Phi instruction correctly merging the returned values
    EXPECT_EQ(b6->instructions.front()->getInstType(), Instruction::Type::Phi);

    std::cout << "\n=== [SchemaBased] Caller AFTER RegAlloc ===\n";
    caller.dump();
}