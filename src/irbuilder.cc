
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

std::string IRBuilder::createAdd(const std::string& lhs, const std::string& rhs) {
    ensureNoTerminator();
    
    auto inst = std::make_unique<BinaryInst>(BinaryInst::Add, lhs, rhs);
    
    inst->name = getNewName();
    std::string name = inst->name;
    
    currentBlock->instructions.push_back(std::move(inst));
    
    return name;
}

std::string IRBuilder::createSub(const std::string& lhs, const std::string& rhs) {
    ensureNoTerminator();
    
    auto inst = std::make_unique<BinaryInst>(BinaryInst::Sub, lhs, rhs);
    
    inst->name = getNewName();
    std::string name = inst->name;
    
    currentBlock->instructions.push_back(std::move(inst));
    
    return name;
}

std::string IRBuilder::createMul(const std::string& lhs, const std::string& rhs) {
    ensureNoTerminator();
    
    auto inst = std::make_unique<BinaryInst>(BinaryInst::Mul, lhs, rhs);
    
    inst->name = getNewName();
    std::string name = inst->name;
    
    currentBlock->instructions.push_back(std::move(inst));
    
    return name;
}

std::string IRBuilder::createICmp(ICmpInst::Pred pred, const std::string& lhs, const std::string& rhs) {
    ensureNoTerminator();
    
    auto inst = std::make_unique<ICmpInst>(pred, lhs, rhs);
    
    inst->name = getNewName();
    std::string name = inst->name;
    
    currentBlock->instructions.push_back(std::move(inst));
    
    return name;
}

std::string IRBuilder::createAlloca() {
    ensureNoTerminator();
    
    auto inst = std::make_unique<AllocaInst>();
    
    inst->name = getNewName();
    std::string name = inst->name;
    
    currentBlock->instructions.push_back(std::move(inst));
    
    return name;
}

std::string IRBuilder::createLoad(const std::string& ptr) {
    ensureNoTerminator();
    
    auto inst = std::make_unique<LoadInst>(ptr);
    
    inst->name = getNewName();
    std::string name = inst->name;
    
    currentBlock->instructions.push_back(std::move(inst));
    
    return name;
}

void IRBuilder::createStore(const std::string& val, const std::string& ptr) {
    ensureNoTerminator();
    currentBlock->instructions.push_back(std::make_unique<StoreInst>(val, ptr));
}

// void IRBuilder::createInstruction(const std::string& code) {
//     if (!currentBlock) throw std::runtime_error("No current block!");
//     ensureNoTerminator();
//     currentBlock->instructions.push_back(std::make_unique<RegularInst>(code));
// }

void IRBuilder::createBr(const std::string& condLabel, const std::string& thenLabel, const std::string& elseLabel) {
    std::vector<std::string> labels = {thenLabel, elseLabel};
    
    std::string code = "br" + condLabel + ", label %" + thenLabel + ", label %" + elseLabel;
    
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
    
    auto phi = std::make_unique<PhiInst>( );
    
    phi->name = getNewName();
    PhiInst* raw = phi.get();
    
    currentBlock->instructions.push_back(std::move(phi));
    
    return raw;
}

void IRBuilder::buildCFG() {
    for (auto& bb : blocks) {
        bb->successors.clear();
        bb->predecessors.clear();
    }
    for (auto& bb : blocks) {
        if (!bb->hasTerminator()) continue;
        auto term = bb->instructions.back().get();
        auto labels = term->getSuccessorLabels();
        for (const auto& label : labels) {
            auto it = blockMap.find(label);
            if (it == blockMap.end()) {
                throw std::runtime_error("Successor block '" + label + "' not found!");
            }
            BasicBlock* succ = it->second;
            bb->successors.push_back(succ);
            succ->predecessors.push_back(bb.get());
        }
    }
}

void IRBuilder::dfsVisit(BasicBlock* node,
              std::unordered_set<BasicBlock*>& visited,
              BasicBlock* skip) {
    if (!node || node == skip || visited.count(node)) return;
    visited.insert(node);
    for (auto* succ : node->successors)
        dfsVisit(succ, visited, skip);
}

std::unordered_map<std::string, std::unordered_set<std::string>> 
IRBuilder::printDominators() {
    buildCFG();
    // if (blocks.empty()) return;

    BasicBlock* entry = blocks.front().get();

    std::unordered_set<BasicBlock*> reachAll;
    dfsVisit(entry, reachAll, nullptr);

    std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>> dominates;

    for (auto& bbPtr : blocks) {
        BasicBlock* b = bbPtr.get();

        if (b == entry) 
            continue;

        std::unordered_set<BasicBlock*> reachWithout;
        dfsVisit(entry, reachWithout, b);

        for (auto* r : reachAll) {
            if (!reachWithout.count(r)) {
                dominates[b].insert(r);
            }
        }

        dominates[b].insert(b);
    }

    for (auto* r : reachAll) 
        dominates[entry].insert(r);

    std::unordered_map<std::string, std::unordered_set<std::string>> result;

    std::cout << "\n=== Dominator ===\n";
    for (auto& bbPtr : blocks) {
        std::unordered_set<std::string> names;
        BasicBlock* b = bbPtr.get();
        
        std::cout << "Block " << b->name << " dominates: { ";
        
        for (auto* d : dominates[b]) {
            std::cout << d->name << " ";
            names.insert(d->name);
        }

        result[b->name] = std::move(names);
        std::cout << "}\n";
    }

    // std::unordered_map<std::string, std::unordered_set<std::string>> result;
    // for (auto& b : blocks) {
    //     std::unordered_set<std::string> names;
    //     for (auto* d : dominates[b.get()]) 
    //         names.insert(d->name);
    //     result[b->name] = std::move(names);
    // }

    return result;

}