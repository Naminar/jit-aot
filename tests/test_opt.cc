#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include "irbuilder.hh"

class GraphVerifier {
private:
    std::unordered_map<Instruction*, Instruction*> instMap;
    std::unordered_map<BasicBlock*, BasicBlock*> blockMap;

public:
    void verify(IRBuilder& actualBuilder, IRBuilder& expectedBuilder) {
        std::vector<BasicBlock*> actBlocks;
        actualBuilder.computeRPO(actBlocks);

        std::vector<BasicBlock*> expBlocks;
        expectedBuilder.computeRPO(expBlocks);

        ASSERT_EQ(actBlocks.size(), expBlocks.size()) 
            << "Mismatch in number of Basic Blocks.";

        for (size_t i = 0; i < actBlocks.size(); ++i) {
            blockMap[actBlocks[i]] = expBlocks[i];
            ASSERT_EQ(actBlocks[i]->name, expBlocks[i]->name) 
                << "Block name mismatch at index " << i;
        }

        for (size_t i = 0; i < actBlocks.size(); ++i) {
            BasicBlock* actBB = actBlocks[i];
            BasicBlock* expBB = expBlocks[i];

            std::vector<Instruction*> actInsts;
            for(auto& ptr : actBB->instructions) {
                if(!ptr->erased) actInsts.push_back(ptr.get());
            }

            std::vector<Instruction*> expInsts;
            for(auto& ptr : expBB->instructions) {
                expInsts.push_back(ptr.get());
            }

            ASSERT_EQ(actInsts.size(), expInsts.size()) 
                << "Instruction count mismatch in block " << actBB->name;

            for (size_t j = 0; j < actInsts.size(); ++j) {
                Instruction* actI = actInsts[j];
                Instruction* expI = expInsts[j];

                instMap[actI] = expI;

                verifyInstruction(actI, expI);
            }
        }
    }

private:
    void verifyOperand(Operand actOp, Operand expOp) {
        ASSERT_EQ(actOp.type, expOp.type) << "Operand type mismatch (Int vs Inst)";
        
        if (actOp.type == Operand::Int) {
            EXPECT_EQ(actOp.intVal, expOp.intVal) << "Constant value mismatch";
        } 
        else if (actOp.type == Operand::Inst) {
            ASSERT_TRUE(instMap.count(actOp.instVal)) 
                << "Found operand instruction that wasn't mapped (processing order issue?)";
            
            EXPECT_EQ(instMap[actOp.instVal], expOp.instVal) 
                << "Dataflow mismatch: Actual instruction uses a different dependency than Expected.";
        }
    }

    void verifyInstruction(Instruction* act, Instruction* exp) {
        ASSERT_EQ(typeid(*act), typeid(*exp)) 
            << "Instruction C++ type mismatch (e.g., BinaryInst vs LoadInst)";

        if (auto* bAct = dynamic_cast<BinaryInst*>(act)) {
            auto* bExp = dynamic_cast<BinaryInst*>(exp);
            EXPECT_EQ(bAct->op, bExp->op) << "Binary Opcode mismatch";
            verifyOperand(bAct->lhs, bExp->lhs);
            verifyOperand(bAct->rhs, bExp->rhs);
        }
        else if (auto* cAct = dynamic_cast<ICmpInst*>(act)) {
            auto* cExp = dynamic_cast<ICmpInst*>(exp);
            EXPECT_EQ(cAct->pred, cExp->pred) << "ICmp Predicate mismatch";
            verifyOperand(cAct->lhs, cExp->lhs);
            verifyOperand(cAct->rhs, cExp->rhs);
        }
        else if (auto* lAct = dynamic_cast<LoadInst*>(act)) {
            auto* lExp = dynamic_cast<LoadInst*>(exp);
            verifyOperand(lAct->ptr, lExp->ptr);
        }
        else if (auto* sAct = dynamic_cast<StoreInst*>(act)) {
            auto* sExp = dynamic_cast<StoreInst*>(exp);
            verifyOperand(sAct->val, sExp->val);
            verifyOperand(sAct->ptr, sExp->ptr);
        }
        else if (auto* tAct = dynamic_cast<TerminatorInst*>(act)) {
            auto* tExp = dynamic_cast<TerminatorInst*>(exp);
            EXPECT_EQ(tAct->type, tExp->type) << "Terminator type mismatch";
            EXPECT_EQ(tAct->succLabels, tExp->succLabels) << "Successor labels mismatch";
            if (tAct->type != TerminatorInst::Br) {
                verifyOperand(tAct->val, tExp->val);
            }
        }
        else if (auto* pAct = dynamic_cast<PhiInst*>(act)) {
            auto* pExp = dynamic_cast<PhiInst*>(exp);
            ASSERT_EQ(pAct->incomings.size(), pExp->incomings.size());
            for(size_t i=0; i<pAct->incomings.size(); ++i) {
                verifyOperand(pAct->incomings[i].val, pExp->incomings[i].val);
                EXPECT_EQ(pAct->incomings[i].blockName, pExp->incomings[i].blockName);
            }
        }
    }
};

void verifyOptimization(IRBuilder& actualBuilder, std::function<void(IRBuilder&)> expectedBuilderSetup) {
    IRBuilder expectedBuilder;
    expectedBuilderSetup(expectedBuilder);
    
    GraphVerifier verifier;
    verifier.verify(actualBuilder, expectedBuilder);
}


TEST(OptimizationTest, ConstantFolding_Sub) {
    IRBuilder builder;
    builder.createBasicBlock("entry");
    
    // %v0 = sub 10, 4  -> should fold to 6
    auto v0 = builder.createSub(10, 4);
    
    // %v1 = sub %v0, 2 -> should fold to 6 - 2 = 4
    auto v1 = builder.createSub(v0, 2);
    
    builder.createRet(v1);

    builder.globalOptimization();

    std::cout << "--- ConstantFolding_Sub Output ---" << std::endl;
    builder.dump();

    // Verification
    verifyOptimization(builder, [](IRBuilder& expected) {
        expected.createBasicBlock("entry");
        expected.createRet(4);
    });
}

TEST(OptimizationTest, ConstantFolding_Chain) {
    IRBuilder builder;
    builder.createBasicBlock("entry");
    
    // v0 = 5 - 2 = 3
    auto v0 = builder.createSub(5, 2);
    // v1 = 3 & 1 = 1
    auto v1 = builder.createAnd(v0, 1);
    // v2 = 8 >> 1 = 4 
    auto v2 = builder.createAShr(8, v1);
    // v3 = 4 + 0 = 4 
    auto v3 = builder.createAdd(v2, 0); 
    
    builder.createRet(v3);

    builder.globalOptimization();
    
    std::cout << "--- ConstantFolding_Chain Output ---" << std::endl;
    builder.dump();

    verifyOptimization(builder, [](IRBuilder& expected) {
        expected.createBasicBlock("entry");
        // Everything folds to 4
        expected.createRet(4);
    });
}

TEST(OptimizationTest, Peephole_Sub) {
    IRBuilder builder;
    builder.createBasicBlock("entry");
    
    auto x = builder.createAlloca();
    
    // Case 1: X - 0 -> X
    auto res1 = builder.createSub(x, 0); 
    
    // Case 2: X - X -> 0
    auto res2 = builder.createSub(x, x); 
    
    auto use1 = builder.createAdd(res1, 1); 
    auto use2 = builder.createAdd(res2, 1); 
    
    builder.createRet();

    builder.globalOptimization();
    
    std::cout << "--- Peephole Sub Test Output ---" << std::endl;
    builder.dump();

    verifyOptimization(builder, [](IRBuilder& expected) {
        expected.createBasicBlock("entry");
        auto x = expected.createAlloca();
        
        expected.createAdd(x, 1);
        
        // expected.createAdd(0, 1);
        
        expected.createRet();
    });
}

TEST(OptimizationTest, Peephole_And) {
    IRBuilder builder;
    builder.createBasicBlock("entry");
    auto x = builder.createAlloca();
    
    // X & 0 -> 0
    auto v1 = builder.createAnd(x, 0);
    // X & X -> X
    auto v2 = builder.createAnd(x, x);
    // X & -1 -> X
    auto v3 = builder.createAnd(x, -1);
    
    auto sum = builder.createAdd(v1, v2);
    auto sum2 = builder.createAdd(sum, v3);
    
    builder.createRet(sum2);
    
    builder.globalOptimization();
    
    std::cout << "--- Peephole And Test Output ---" << std::endl;
    builder.dump();

    verifyOptimization(builder, [](IRBuilder& expected) {
        expected.createBasicBlock("entry");
        auto x = expected.createAlloca();
        
        // v1 -> 0, v2 -> x
        // sum = 0 + x
        auto sum = expected.createAdd(0, x);
        
        // v3 -> x
        // sum2 = sum + x
        auto sum2 = expected.createAdd(sum, x);
        
        expected.createRet(sum2);
    });
}

TEST(OptimizationTest, Peephole_AShr) {
    IRBuilder builder;
    builder.createBasicBlock("entry");
    auto x = builder.createAlloca();
    
    // X >> 0 -> X
    auto v1 = builder.createAShr(x, 0);
    // 0 >> X -> 0
    auto v2 = builder.createAShr(0, x);
    
    auto sum = builder.createAdd(v1, v2);
    builder.createRet(sum);
    
    builder.globalOptimization();
    
    std::cout << "--- Peephole AShr Test Output ---" << std::endl;
    builder.dump();

    verifyOptimization(builder, [](IRBuilder& expected) {
        expected.createBasicBlock("entry");
        auto x = expected.createAlloca();
        
        // v1 -> x, v2 -> 0
        // sum = x + 0
        auto sum = expected.createAdd(x, 0);
        
        expected.createRet(sum);
    });
}

TEST(OptimizationTest, GraphReduction) {
    IRBuilder builder;
    builder.createBasicBlock("entry");
    
    auto c1 = builder.createSub(10, 2); // 8
    auto c2 = builder.createAShr(c1, 2);  // 8 >> 2 = 2
    auto c3 = builder.createAnd(c2, 3);   // 2 & 3 = 2
    
    auto x = builder.createAlloca();
    auto result = builder.createAdd(x, c3); // x + 2
    builder.createRet(result);
    
    builder.globalOptimization();
    
    std::cout << "--- Graph Reduction Test Output ---" << std::endl;
    builder.dump();
    
    verifyOptimization(builder, [](IRBuilder& expected) {
        expected.createBasicBlock("entry");
        auto x = expected.createAlloca();

        auto result = expected.createAdd(x, 2);
        expected.createRet(result);
    });
}