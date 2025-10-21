#include <unordered_set>
#include <unordered_map>

#include "ins.hh"

class BasicBlock {
public:
    std::string name;
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::vector<BasicBlock*> successors;
    std::vector<BasicBlock*> predecessors;

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

class IRBuilder {
private:
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    std::unordered_map<std::string, BasicBlock*> blockMap;
    BasicBlock* currentBlock = nullptr;
    size_t nextValueID = 0;

    void ensureNoTerminator();

    std::string getNewName() {
        return "%v" + std::to_string(nextValueID++);
    }

public:
    BasicBlock* createBasicBlock(const std::string& name);

    void setInsertPoint(BasicBlock* bb) {
        currentBlock = bb;
    }

    // void createInstruction(const std::string& code);

    std::string createAdd(const std::string& lhs, const std::string& rhs);

    std::string createSub(const std::string& lhs, const std::string& rhs);

    std::string createMul(const std::string& lhs, const std::string& rhs);

    std::string createICmp(ICmpInst::Pred pred, const std::string& lhs, const std::string& rhs);

    std::string createAlloca();

    std::string createLoad(const std::string& ptr);

    void createStore(const std::string& val, const std::string& ptr);

    void createBr(const std::string& condLabel, const std::string& thenLabel, const std::string& elseLabel);

    void createBr(const std::string& targetLabel);

    void createRet(const std::string& val = "");

    PhiInst* createPHI();

    void buildCFG();

    void dfsVisit(BasicBlock* node,
              std::unordered_set<BasicBlock*>& visited,
              BasicBlock* skip = nullptr);
    
    std::unordered_map<std::string, std::unordered_set<std::string>> printDominators();

    void dump() const {
        for (const auto& bb : blocks) {
            bb->print();
        }
    }

};