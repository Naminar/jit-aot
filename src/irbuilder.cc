
#include "irbuilder.hh"

void IRBuilder::ensureNoTerminator() {
    if (currentBlock && currentBlock->hasTerminator()) {
        throw std::runtime_error("Impossible to insert instruction after terminator in block '" + currentBlock->name + "'");
    }
}

BasicBlock* IRBuilder::createBasicBlock(const std::string& name) {
    if (blockMap.count(name)) {
        throw std::runtime_error("Block '" + name + "' already exists!");
    }
    auto bb = std::make_unique<BasicBlock>(name);
    currentBlock = bb.get();
    blockMap[name] = currentBlock;
    blocks.push_back(std::move(bb));
    return currentBlock;
}

void IRBuilder::createInstruction(const std::string& code) {
    if (!currentBlock) throw std::runtime_error("No current block!");
    ensureNoTerminator();
    currentBlock->instructions.push_back(std::make_unique<RegularInst>(code));
}

void IRBuilder::createBr(const std::string& condLabel, const std::string& thenLabel, const std::string& elseLabel) {
    std::vector<std::string> labels = {thenLabel, elseLabel};
    std::string code = "br i1 " + condLabel + ", label %" + thenLabel + ", label %" + elseLabel;
    currentBlock->instructions.push_back(std::make_unique<TerminatorInst>(code, labels));
}

void IRBuilder::createBr(const std::string& targetLabel) {
    std::vector<std::string> labels = {targetLabel};
    std::string code = "br label %" + targetLabel;
    currentBlock->instructions.push_back(std::make_unique<TerminatorInst>(code, labels));
}

void IRBuilder::createRet(const std::string& val) {
    std::string code = val.empty() ? "ret void" : ("ret i32 " + val);
    std::vector<std::string> label = {};
    currentBlock->instructions.push_back(std::make_unique<TerminatorInst>(code, label));
}

PhiInst* IRBuilder::createPHI() {
    if (!currentBlock) 
        throw std::runtime_error("No current block!");
    ensureNoTerminator();
    auto phi = std::make_unique<PhiInst>();
    PhiInst* raw = phi.get();
    currentBlock->instructions.push_back(std::move(phi));
    return raw;
}