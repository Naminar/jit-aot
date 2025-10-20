#include <unordered_set>
#include <unordered_map>

#include "ins.hh"

class BasicBlock {
public:
    std::string name;
    std::vector<std::unique_ptr<Instruction>> instructions;

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

    void ensureNoTerminator();

public:
    BasicBlock* createBasicBlock(const std::string& name);

    void setInsertPoint(BasicBlock* bb) {
        currentBlock = bb;
    }

    void createInstruction(const std::string& code);

    void createBr(const std::string& condLabel, const std::string& thenLabel, const std::string& elseLabel);

    void createBr(const std::string& targetLabel);

    void createRet(const std::string& val = "");

    PhiInst* createPHI();

    void dump() const {
        for (const auto& bb : blocks) {
            bb->print();
        }
    }

};