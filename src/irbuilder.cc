
#include "irbuilder.hh"

#include <functional>
#include <algorithm>

void IRBuilder::ensureNoTerminator() {
    if (currentBlock && currentBlock->hasTerminator()) {
        throw std::runtime_error("Impossible to insert instruction after terminator in block '" 
                                    + currentBlock->name + "'");
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


Instruction* IRBuilder::createAdd(Operand lhs, Operand rhs) {
    ensureNoTerminator();

    auto inst = std::make_unique<BinaryInst>(BinaryInst::Add, lhs, rhs);

    inst->name = getNewName();
    Instruction* ptr = inst.get();

    currentBlock->instructions.push_back(std::move(inst));

    return ptr;
}


Instruction* IRBuilder::createSub(Operand lhs, Operand rhs) {
    ensureNoTerminator();

    auto inst = std::make_unique<BinaryInst>(BinaryInst::Sub, lhs, rhs);

    inst->name = getNewName();
    Instruction* ptr = inst.get();

    currentBlock->instructions.push_back(std::move(inst));

    return ptr;
}


Instruction* IRBuilder::createMul(Operand lhs, Operand rhs) {
    ensureNoTerminator();

    auto inst = std::make_unique<BinaryInst>(BinaryInst::Mul, lhs, rhs);

    inst->name = getNewName();
    Instruction* ptr = inst.get();

    currentBlock->instructions.push_back(std::move(inst));

    return ptr;
}

Instruction* IRBuilder::createAnd(Operand lhs, Operand rhs) {
    ensureNoTerminator();

    auto inst = std::make_unique<BinaryInst>(BinaryInst::And, lhs, rhs);

    inst->name = getNewName();
    Instruction* ptr = inst.get();

    currentBlock->instructions.push_back(std::move(inst));

    return ptr;
}

Instruction* IRBuilder::createAShr(Operand lhs, Operand rhs) {
    ensureNoTerminator();

    auto inst = std::make_unique<BinaryInst>(BinaryInst::AShr, lhs, rhs);

    inst->name = getNewName();
    Instruction* ptr = inst.get();

    currentBlock->instructions.push_back(std::move(inst));

    return ptr;
}

Instruction* IRBuilder::createICmp(ICmpInst::Pred pred, Operand lhs, Operand rhs) {
    ensureNoTerminator();

    auto inst = std::make_unique<ICmpInst>(pred, lhs, rhs);

    inst->name = getNewName();
    Instruction* ptr = inst.get();

    currentBlock->instructions.push_back(std::move(inst));

    return ptr;
}


Instruction* IRBuilder::createAlloca() {
    ensureNoTerminator();

    auto inst = std::make_unique<AllocaInst>();

    inst->name = getNewName();
    Instruction* ptr = inst.get();

    currentBlock->instructions.push_back(std::move(inst));

    return ptr;
}


Instruction* IRBuilder::createLoad(Operand ptr) {
    ensureNoTerminator();

    auto inst = std::make_unique<LoadInst>(ptr);

    inst->name = getNewName();
    Instruction* ptr_ = inst.get();

    currentBlock->instructions.push_back(std::move(inst));

    return ptr_;
}


void IRBuilder::createStore(Operand val, Operand ptr) {
    ensureNoTerminator();
    currentBlock->instructions.push_back(std::make_unique<StoreInst>(val, ptr));
}


void IRBuilder::createBr(Operand cond, const std::string& thenLabel, const std::string& elseLabel) {
    std::vector<std::string> labels = {thenLabel, elseLabel};
    currentBlock->instructions.push_back(
        std::make_unique<TerminatorInst>(TerminatorInst::BrCond, cond, labels));
}


void IRBuilder::createBr(const std::string& targetLabel) {
    std::vector<std::string> labels = {targetLabel};
    currentBlock->instructions.push_back(
        std::make_unique<TerminatorInst>(TerminatorInst::Br, Operand(), labels));
}


void IRBuilder::createRet(Operand val) {
    currentBlock->instructions.push_back(
        std::make_unique<TerminatorInst>(TerminatorInst::Ret, val, std::vector<std::string>{}));
}

void IRBuilder::createRet() {
    createRet(Operand());
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
        if (!bb->hasTerminator()) 
            continue;
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
    if (!node || node == skip || visited.count(node)) 
        return;
    visited.insert(node);

    for (auto* succ : node->successors)
        dfsVisit(succ, visited, skip);
}


// std::unordered_map<std::string, std::unordered_set<std::string>>
void IRBuilder::computeDominators() {
    buildCFG();
    dominates.clear();

    if (blocks.empty())
        return;
    BasicBlock* entry = blocks.front().get();

    std::unordered_set<BasicBlock*> reachAll;
    dfsVisit(entry, reachAll, nullptr);

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
}


std::unordered_map<std::string, std::unordered_set<std::string>>
IRBuilder::printDominators() {
    computeDominators();
    // if (blocks.empty()) return;

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

    return result;
}


void IRBuilder::computeRPO(std::vector<BasicBlock*>& outRPO) {
    outRPO.clear();
    if (blocks.empty()) 
        return;
    BasicBlock* entry = blocks.front().get();
    std::unordered_set<BasicBlock*> vis;
    std::vector<BasicBlock*> postorder;

    std::function<void(BasicBlock*)> dfs = [&](BasicBlock* n) {
        if (!n || vis.count(n))
            return;
        vis.insert(n);

        for (auto* s : n->successors)
            dfs(s);
        postorder.push_back(n);
    };

    dfs(entry);
    outRPO = postorder;
    std::reverse(outRPO.begin(), outRPO.end());
}


void IRBuilder::collectBackEdges(
    std::unordered_map<BasicBlock*,
    std::vector<BasicBlock*>> &backEdgesByHeader,
    std::unordered_map<BasicBlock*, bool> &isIrreducibleFlag
) {
    backEdgesByHeader.clear();
    isIrreducibleFlag.clear();

    if (blocks.empty()) 
        return;
    BasicBlock* entry = blocks.front().get();

    enum Color { White=0, Gray=1, Black=2 };

    std::unordered_map<BasicBlock*, Color> color;
    for (auto& bbPtr : blocks) color[bbPtr.get()] = White;

    std::function<void(BasicBlock*)> dfs = [&](BasicBlock* n) {
        if (!n) 
            return;
        color[n] = Gray;
    
        for (auto* s : n->successors) {
            if (color[s] == White) {
                dfs(s);
            } else if (color[s] == Gray) {
                backEdgesByHeader[s].push_back(n);

                bool headerDominatesSource = false;

                if (dominates.count(s)) {
                    headerDominatesSource = dominates[s].count(n) > 0;
                } else {
                    headerDominatesSource = false;
                }
                if (!headerDominatesSource)
                    isIrreducibleFlag[s] = true;
            } else {
            }
        }
        color[n] = Black;
    };

    dfs(entry);
}


void IRBuilder::analyzeLoops() {
    computeDominators();

    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> backEdgesByHeader;
    std::unordered_map<BasicBlock*, bool> isIrreducibleFlag;
    collectBackEdges(backEdgesByHeader, isIrreducibleFlag);

    std::vector<BasicBlock*> rpo;
    computeRPO(rpo);

    std::unordered_map<BasicBlock*, std::unique_ptr<Loop>> loopsByHeader;
    // std::vector<Loop*> loopList;

    for (auto& kv : backEdgesByHeader) {
        BasicBlock* header = kv.first;
        loopsByHeader[header] = std::make_unique<Loop>(header);
        // loopList.push_back(loopsByHeader[header].get());
        allLoops.push_back(std::move(loopsByHeader[header]));
        loopList.push_back(allLoops.back().get());
    }

    for (auto* hdr : loopList) {
        hdr->backEdges = backEdgesByHeader[hdr->header];
        hdr->irreducible = (isIrreducibleFlag.count(hdr->header) 
                            && isIrreducibleFlag[hdr->header]);

        hdr->blocks.insert(hdr->header);
        hdr->header->parentLoop = hdr;
    }

    auto assignBlockToLoop = [&](Loop* L, BasicBlock* B) {
        if (!L || !B)
            return;

        if (B->parentLoop == L)
            return;

            if (B->parentLoop != nullptr && B->parentLoop != L) {
                L->addInnerLoop(B->parentLoop);
                return;
            }
        L->addBlock(B);
    };

    std::unordered_map<BasicBlock*, int> rpoIndex;
    for (size_t i = 0; i < rpo.size(); ++i) 
        rpoIndex[rpo[i]] = (int)i;

    std::sort(loopList.begin(), loopList.end(), [&](Loop* a, Loop* b) {
        int ia = rpoIndex.count(a->header) ? rpoIndex[a->header] : -1;
        int ib = rpoIndex.count(b->header) ? rpoIndex[b->header] : -1;
        return ia > ib;
    });

    for (Loop* L : loopList) {
        BasicBlock* header = L->header;
        if (L->irreducible) {
            std::unordered_set<BasicBlock*> reachable_forward;
            std::stack<BasicBlock*> fwd_stack;

            fwd_stack.push(header);
            reachable_forward.insert(header);

            while(!fwd_stack.empty()) {
                BasicBlock* current = fwd_stack.top(); fwd_stack.pop();
                
                for (auto* succ : current->successors) {
                    if (reachable_forward.find(succ) == reachable_forward.end()) {
                        reachable_forward.insert(succ);
                        fwd_stack.push(succ);
                    }
                }
            }

            std::unordered_set<BasicBlock*> reachable_backward;
            std::stack<BasicBlock*> bwd_stack;

            bwd_stack.push(header);
            reachable_backward.insert(header);

            while(!bwd_stack.empty()) {
                BasicBlock* current = bwd_stack.top(); 
                bwd_stack.pop();
                
                for (auto* pred : current->predecessors) {
                    if (reachable_backward.find(pred) == reachable_backward.end()) {
                        reachable_backward.insert(pred);
                        bwd_stack.push(pred);
                    }
                }
            }

            for (BasicBlock* block : reachable_forward) {
                if (reachable_backward.count(block)) {
                    assignBlockToLoop(L, block);
                }
            }

        } else {
            std::unordered_set<BasicBlock*> visited;
            visited.insert(header);

            for (auto* src : L->backEdges) {
                std::stack<BasicBlock*> st;
                if (!visited.count(src)) {
                    st.push(src);
                    while (!st.empty()) {
                        BasicBlock* cur = st.top(); 
                        st.pop();
                        
                        if (visited.count(cur)) 
                            continue;
                        
                        visited.insert(cur);

                        if (cur->parentLoop == nullptr) {
                            assignBlockToLoop(L, cur);
                        } else if (cur->parentLoop != L) {
                            if (cur->parentLoop->outerLoop == nullptr) {
                                L->addInnerLoop(cur->parentLoop);
                            } else if (cur->parentLoop->outerLoop != L) {}
                        }

                        for (auto* pred : cur->predecessors) {
                            if (!visited.count(pred)) {
                                st.push(pred);
                            }
                        }
                    }
                } else {
                    if (src->parentLoop == nullptr) assignBlockToLoop(L, src);
                }
            }

            assignBlockToLoop(L, header);
        }
    }

    auto rootLoop = std::make_unique<Loop>(nullptr);

    for (auto& bbPtr : blocks) {
        BasicBlock* b = bbPtr.get();
        if (b->parentLoop == nullptr) {
            rootLoop->addBlock(b);
        }
    }

    for (Loop* L : loopList) {
        if (L->outerLoop == nullptr) {
            rootLoop->addInnerLoop(L);
        }
    }

    std::cout << "\n=== Loops (" << loopList.size() << ") ===\n";
    int idx = 0;
    for (Loop* L : loopList) {
        std::cout << "Loop #" << (++idx) << ":\n";
        L->print(2);
        std::cout << "\n";
    }

    std::cout << "Root loop (blocks not in any loop):\n";
    rootLoop->print(2);
}


void IRBuilder::replaceInstruction(Instruction* oldInst, Operand newVal) {

    std::vector<Instruction*> currentUsers = oldInst->users;
    for (Instruction* user : currentUsers) {
        user->replaceOperand(oldInst, newVal);
    }
    oldInst->users.clear();

    oldInst->dropOperands();
    
    oldInst->erased = true;
}

Operand IRBuilder::foldInstruction(Instruction* inst) {
    auto* bin = dynamic_cast<BinaryInst*>(inst);
    if (!bin) return Operand();

    if (bin->lhs.type == Operand::Int && bin->rhs.type == Operand::Int) {
        int l = bin->lhs.intVal;
        int r = bin->rhs.intVal;
        int result = 0;
        
        switch(bin->op) {
            case BinaryInst::Sub:  result = l - r; break;
            case BinaryInst::And:  result = l & r; break;
            case BinaryInst::AShr: result = l >> r; break;

            case BinaryInst::Add:  result = l + r; break;
            case BinaryInst::Mul:  result = l * r; break;
        }
        return Operand(result);
    }
    return Operand();
}

Operand IRBuilder::peepholeInstruction(Instruction* inst) {
    auto* bin = dynamic_cast<BinaryInst*>(inst);
    if (!bin) return Operand();

    // Sub
    if (bin->op == BinaryInst::Sub) {
        // x - 0 -> x
        if (bin->rhs.type == Operand::Int && bin->rhs.intVal == 0) {
            return bin->lhs;
        }
        // x - x -> 0
        if (bin->lhs == bin->rhs) {
            return Operand(0);
        }
    }
    
    // And
    if (bin->op == BinaryInst::And) {
        // x & 0 -> 0 or 0 & x -> 0
        if ((bin->rhs.type == Operand::Int && bin->rhs.intVal == 0) 
            ||
            (bin->lhs.type == Operand::Int && bin->lhs.intVal == 0)) {
            return Operand(0);
        }
        // x & -1 -> x
        if (bin->rhs.type == Operand::Int && bin->rhs.intVal == -1) {
            return bin->lhs;
        }
        if (bin->lhs.type == Operand::Int && bin->lhs.intVal == -1) {
            return bin->rhs;
        }
        // x & x -> x
        if (bin->lhs == bin->rhs) {
            return bin->lhs;
        }
    }

    // AShr
    if (bin->op == BinaryInst::AShr) {
        // x >> 0 -> x
        if (bin->rhs.type == Operand::Int && bin->rhs.intVal == 0) {
            return bin->lhs;
        }
        // 0 >> x -> 0
        if (bin->lhs.type == Operand::Int && bin->lhs.intVal == 0) {
            return Operand(0);
        }
    }

    return Operand();
}

void IRBuilder::globalOptimization() {
    std::vector<BasicBlock*> rpo;
    computeRPO(rpo);

    for (BasicBlock* bb : rpo) {
        for (auto& instPtr : bb->instructions) {
            Instruction* inst = instPtr.get();
            if (!inst || inst->erased) continue;

            // Constant Folding
            Operand folded = foldInstruction(inst);
            if (folded.type != Operand::Undef) {
                replaceInstruction(inst, folded);
                continue; 
            }

            // Peephole
            Operand peep = peepholeInstruction(inst);
            if (peep.type != Operand::Undef) {
                replaceInstruction(inst, peep);
                continue;
            }
        }

        bb->instructions.erase(
            std::remove_if(bb->instructions.begin(), bb->instructions.end(),
                [](const std::unique_ptr<Instruction>& inst) {
                    return inst->erased;
                }),
            bb->instructions.end()
        );
    }
}