#include <unordered_set>
#include <unordered_map>
#include <stack>

#include "ins.hh"

class Loop;

class BasicBlock {
public:
    std::string name;
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::vector<BasicBlock*> successors;
    std::vector<BasicBlock*> predecessors;

    Loop* parentLoop = nullptr;

    BasicBlock(const std::string& n) : name(n) {}

    bool hasTerminator() const {
        return !instructions.empty() && 
               instructions.back()->getInstType() == Instruction::Type::Terminator;
    }

    void print() const {
        std::cout << "block " << name << ":\n";
        for (const auto& inst : instructions) {
            inst->print();
        }
        if (!hasTerminator()) {
            std::cout << "  ; WARNING: missing terminator!\n";
        }
        std::cout << "\n";
    }
};

struct LoopInfoExpected {
    std::string header;
    std::unordered_set<std::string> backEdges;
    std::unordered_set<std::string> blocks;
    bool isIrreducible;
};

class Loop {
public:
    BasicBlock* header = nullptr;
    std::vector<BasicBlock*> backEdges;
    std::unordered_set<BasicBlock*> blocks;
    std::vector<Loop*> innerLoops;
    Loop* outerLoop = nullptr;
    bool irreducible = false;

    Loop(BasicBlock* h = nullptr) : header(h) {
        if (h) blocks.insert(h);
    }

    void addBlock(BasicBlock* bb) {
        blocks.insert(bb);
        bb->parentLoop = this;
    }

    void addInnerLoop(Loop* inner) {
        if (!inner) return;
        inner->outerLoop = this;
        innerLoops.push_back(inner);
    }

    void print(int indent = 0) const {
        std::string pad(indent, ' ');
        std::cout << pad << "Loop header: " << (header ? header->name : std::string("<null>")) 
                  << (irreducible ? " (irreducible)" : "") << "\n";
        std::cout << pad << "  Back edges from: { ";
        for (auto* b : backEdges) std::cout << b->name << " ";
        std::cout << "}\n";
        std::cout << pad << "  Blocks: { ";
        for (auto* b : blocks) std::cout << b->name << " ";
        std::cout << "}\n";
        if (!innerLoops.empty()) {
            std::cout << pad << "  Inner loops:\n";
            for (auto* il : innerLoops) il->print(indent + 4);
        }
    }

    LoopInfoExpected makeLoopExpected(const std::string& headerName,
        const std::initializer_list<std::string>& backEdges,
        const std::initializer_list<std::string>& blocks, bool irreducible = false) 
    {
        LoopInfoExpected loop;
        loop.header = headerName;
        loop.backEdges = std::unordered_set<std::string>(backEdges.begin(), backEdges.end());
        loop.blocks = std::unordered_set<std::string>(blocks.begin(), blocks.end());
        loop.isIrreducible = irreducible;
        return loop;
    }

};

class IRBuilder {
private:
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    std::unordered_map<std::string, BasicBlock*> blockMap;
    BasicBlock* currentBlock = nullptr;
    size_t nextValueID = 0;
    // std::vector<Loop*> loopList;

    std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>> dominates;

    void ensureNoTerminator();

    std::string getNewName() {
        return "%v" + std::to_string(nextValueID++);
    }

    // Optimization helpers
    void replaceInstruction(Instruction* oldInst, Operand newVal);
    Operand foldInstruction(Instruction* inst);
    Operand peepholeInstruction(Instruction* inst);

public:
    std::vector<Loop*> loopList;
    std::vector<std::unique_ptr<Loop>> allLoops;
    
    BasicBlock* createBasicBlock(const std::string& name);

    void setInsertPoint(BasicBlock* bb) {
        currentBlock = bb;
    }

    Instruction* createAdd(Operand lhs, Operand rhs);

    Instruction* createSub(Operand lhs, Operand rhs);

    Instruction* createMul(Operand lhs, Operand rhs);

    Instruction* createAnd(Operand lhs, Operand rhs);

    Instruction* createAShr(Operand lhs, Operand rhs);

    Instruction* createICmp(ICmpInst::Pred pred, Operand lhs, Operand rhs);

    Instruction* createAlloca();

    Instruction* createLoad(Operand ptr);

    void createStore(Operand val, Operand ptr);

    void createBr(Operand cond, const std::string& thenLabel, const std::string& elseLabel);

    void createBr(const std::string& targetLabel);

    void createRet(Operand val);
    
    void createRet();

    PhiInst* createPHI();

    void buildCFG();

    void dfsVisit(BasicBlock* node,
              std::unordered_set<BasicBlock*>& visited,
              BasicBlock* skip = nullptr);
    
    void dump() const {
        for (const auto& bb : blocks) {
            bb->print();
        }
    }

    void computeDominators();

    void computeRPO(std::vector<BasicBlock*>& outRPO);

    void analyzeLoops();

    void collectBackEdges( std::unordered_map<BasicBlock*, 
        std::vector<BasicBlock*>> &backEdgesByHeader, std::unordered_map<BasicBlock*, bool> &isIrreducibleFlag);
    
    std::unordered_map<std::string, std::unordered_set<std::string>> 
    printDominators();

    void globalOptimization();
};