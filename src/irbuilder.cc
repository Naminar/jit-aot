
#include "irbuilder.hh"

#include <functional>


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
    if (blocks.empty()) return;
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

    if (blocks.empty()) return;
    BasicBlock* entry = blocks.front().get();

    enum Color { White=0, Gray=1, Black=2 };
    std::unordered_map<BasicBlock*, Color> color;
    for (auto& bbPtr : blocks) color[bbPtr.get()] = White;

    std::function<void(BasicBlock*)> dfs = [&](BasicBlock* n) {
        if (!n) return;
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
                if (!headerDominatesSource) isIrreducibleFlag[s] = true;
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
        hdr->irreducible = (isIrreducibleFlag.count(hdr->header) && isIrreducibleFlag[hdr->header]);

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
    for (size_t i = 0; i < rpo.size(); ++i) rpoIndex[rpo[i]] = (int)i;

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
                BasicBlock* current = bwd_stack.top(); bwd_stack.pop();
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
                        BasicBlock* cur = st.top(); st.pop();
                        if (visited.count(cur)) continue;
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
