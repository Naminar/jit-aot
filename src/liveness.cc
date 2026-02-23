
#include "irbuilder.hh"

#include <functional>
#include <algorithm>

void IRBuilder::computeLinearOrder() {
    // ---  FIX DANGLING POINTERS FROM analyzeLoops() ---
    std::unordered_set<Loop*> validLoops;
    for (auto& l : allLoops) validLoops.insert(l.get());
    
    for (auto& bbPtr : blocks) {
        if (validLoops.find(bbPtr->parentLoop) == validLoops.end()) {
            bbPtr->parentLoop = nullptr;
        }
    }
    for (auto& l : allLoops) {
        if (validLoops.find(l->outerLoop) == validLoops.end()) {
            l->outerLoop = nullptr;
        }
    }
    // ---  FIX DANGLING POINTERS FROM analyzeLoops() ---

    std::vector<BasicBlock*> rpo;
    computeRPO(rpo);
    
    std::unordered_map<BasicBlock*, int> rpoIdx;
    for (size_t i = 0; i < rpo.size(); ++i) {
        rpoIdx[rpo[i]] = static_cast<int>(i);
    }

    linearOrder.clear();

    // Loops are grouped together contiguously
    std::function<void(Loop*)> emitLoop = [&](Loop* L) {
        struct Node {
            BasicBlock* b; 
            Loop* l; 
            int idx;
            bool operator<(const Node& o) const { return idx < o.idx; }
        };
        std::vector<Node> nodes;
        
        if (L == nullptr) {
            for (auto& bbPtr : blocks) {
                if (bbPtr->parentLoop == nullptr) {
                    nodes.push_back({bbPtr.get(), nullptr, rpoIdx[bbPtr.get()]});
                }
            }
            for (Loop* loop : loopList) {
                if (loop->outerLoop == nullptr) {
                    nodes.push_back({nullptr, loop, rpoIdx[loop->header]});
                }
            }
        } else {
            for (BasicBlock* b : L->blocks) {
                if (b->parentLoop == L) {
                    nodes.push_back({b, nullptr, rpoIdx[b]});
                }
            }
            for (Loop* inner : L->innerLoops) {
                nodes.push_back({nullptr, inner, rpoIdx[inner->header]});
            }
        }
        
        std::sort(nodes.begin(), nodes.end());
        for (auto& n : nodes) {
            if (n.b) linearOrder.push_back(n.b);
            else emitLoop(n.l);
        }
    };
    
    emitLoop(nullptr);

    int currentId = 0;
    int line = 0;
    for (BasicBlock* b : linearOrder) {
        b->from = currentId;
        
        currentId += 2;

        for (auto& instPtr : b->instructions) {
            instPtr->line = line;
            line += 1;

            if (instPtr->getInstType() == Instruction::Type::Phi) {
                // PHI instructions
                instPtr->id = b->from;
            } else {
                // Regular instructions
                instPtr->id = currentId;
                currentId += 2;
            }
        }
        b->to = currentId;
    }
}



/*
In linear order:
    live = union of successor.liveIn
    for each phi function phi of successors of b: live.add(phi.inputOf(b))
    for each opd in live do intervals[opd].addRange(b.from, b.to)
    for each opd in live do intervals[opd].addRange(b.from, b.to)
    for each operation op of b in reverse order
        [Look at the code description]
        ...
    for each phi function phi of b do live.remove(phi.output)
    if b is loop header then extend entire loop scope
*/
void IRBuilder::computeLiveness() {
    intervals.clear();

    for (auto it = linearOrder.rbegin(); it != linearOrder.rend(); ++it) {
        BasicBlock* b = *it;
        std::unordered_set<Instruction*> live;

        for (BasicBlock* succ : b->successors) {
            for (Instruction* inst : succ->liveIn) {
                live.insert(inst);
            }
        }

        for (BasicBlock* succ : b->successors) {
            for (auto& instPtr : succ->instructions) {
                if (auto phi = dynamic_cast<PhiInst*>(instPtr.get())) {
                    for (auto& inc : phi->incomings) {
                        if (inc.blockName == b->name && inc.val.type == Operand::Inst) {
                            live.insert(inc.val.instVal);
                        }
                    }
                }
            }
        }

        for (Instruction* opd : live) {
            if (opd) intervals[opd].addRange(b->from, b->to);
        }

        // for each operation op of b in reverse order
        for (auto instIt = b->instructions.rbegin(); instIt != b->instructions.rend(); ++instIt) {
            Instruction* op = instIt->get();
            
            // Phi functions processed separately
            if (op->getInstType() == Instruction::Type::Phi) {
                continue;
            }

            // Definitions (outputs) shorten interval and kill liveness
            if (op->getInstType() != Instruction::Type::Terminator && dynamic_cast<StoreInst*>(op) == nullptr) {
                intervals[op].setFrom(op->id);
                live.erase(op);
            }

            // Uses (inputs) create/extend live range
            auto processInput = [&](const Operand& opd) {
                if (opd.type == Operand::Inst && opd.instVal) {
                    intervals[opd.instVal].addRange(b->from, op->id);
                    live.insert(opd.instVal);
                }
            };

            if (auto bin = dynamic_cast<BinaryInst*>(op)) {
                processInput(bin->lhs); processInput(bin->rhs);
            } else if (auto cmp = dynamic_cast<ICmpInst*>(op)) {
                processInput(cmp->lhs); processInput(cmp->rhs);
            } else if (auto load = dynamic_cast<LoadInst*>(op)) {
                processInput(load->ptr);
            } else if (auto store = dynamic_cast<StoreInst*>(op)) {
                processInput(store->val); processInput(store->ptr);
            } else if (auto term = dynamic_cast<TerminatorInst*>(op)) {
                processInput(term->val);
            }
        }

        for (auto& instPtr : b->instructions) {
            if (auto phi = dynamic_cast<PhiInst*>(instPtr.get())) {
                live.erase(phi);
            }
        }

        if (b->parentLoop && b->parentLoop->header == b) {
            int loopEndTo = b->to;
            for (BasicBlock* loopBlock : b->parentLoop->blocks) {
                loopEndTo = std::max(loopEndTo, loopBlock->to);
            }
            for (Instruction* opd : live) {
                if (opd) intervals[opd].addRange(b->from, loopEndTo);
            }
        }

        b->liveIn = live;
    }
}

void IRBuilder::printLinearOrder() {
    std::cout << "\n=== Linear Block Order & IDs ===\n";
    for (BasicBlock* b : linearOrder) {
        std::cout << "Block " << b->name << " [bounds: " << b->from << " to " << b->to << "]\n";
        for (auto& instPtr : b->instructions) {
            std::cout << "line  " << instPtr->line << ", live  " << instPtr->id << ": ";
            instPtr->print();
        }
    }
}

void IRBuilder::printLiveness() {
    std::cout << "\n=== Liveness Intervals ===\n";
    for (const auto& kv : intervals) {
        std::cout << kv.first->name << " : ";
        for (const auto& r : kv.second.ranges) {
            std::cout << "[" << r.start << ", " << r.end << ") ";
        }
        std::cout << "\n";
    }
}