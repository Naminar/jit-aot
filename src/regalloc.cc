
#include "irbuilder.hh"
#include <algorithm>
#include <set>

void IRBuilder::allocateRegisters(int numRegs) {
    std::vector<LiveInterval*> sortedIntervals;
    
    for (auto& kv : intervals) {
        kv.first->allocatedReg = -1;
        kv.first->allocatedStackSlot = -1;
        
        if (!kv.second.ranges.empty()) {
            kv.second.reg = kv.first; 
            sortedIntervals.push_back(&kv.second);
        }
    }

    // Sort by start time of the first range
    std::sort(sortedIntervals.begin(), sortedIntervals.end(), [](LiveInterval* a, LiveInterval* b) {
        return a->ranges.front().start < b->ranges.front().start;
    });

    std::vector<LiveInterval*> active;
    std::set<int> freeRegs;
    for (int i = 0; i < numRegs; ++i) {
        freeRegs.insert(i);
    }
    
    int nextStackSlot = 0;

    for (LiveInterval* i : sortedIntervals) {
        // Expire old intervals
        for (auto it = active.begin(); it != active.end(); ) {
            LiveInterval* j = *it;
            if (j->ranges.back().end <= i->ranges.front().start) {
                freeRegs.insert(j->reg->allocatedReg);
                it = active.erase(it);
            } else {
                ++it;
            }
        }

        if (active.size() == static_cast<size_t>(numRegs)) {
            if (numRegs == 0) {
                i->reg->allocatedStackSlot = nextStackSlot++;
                continue;
            }
            
            // Interval that ends latest
            LiveInterval* spill = active.back(); 
            
            if (spill->ranges.back().end > i->ranges.back().end) {
                i->reg->allocatedReg = spill->reg->allocatedReg;
                
                spill->reg->allocatedReg = -1;
                spill->reg->allocatedStackSlot = nextStackSlot++;
                
                active.back() = i; 
                std::sort(active.begin(), active.end(), [](LiveInterval* a, LiveInterval* b) {
                    return a->ranges.back().end < b->ranges.back().end;
                });
            } else {
                i->reg->allocatedStackSlot = nextStackSlot++;
            }
        } else {
            int reg = *freeRegs.begin();
            freeRegs.erase(freeRegs.begin());
            
            i->reg->allocatedReg = reg;
            
            active.push_back(i);
            std::sort(active.begin(), active.end(), [](LiveInterval* a, LiveInterval* b) {
                return a->ranges.back().end < b->ranges.back().end;
            });
        }
    }

    insertSpillFillInstructions();
}

void IRBuilder::insertSpillFillInstructions() {
    for (auto& bbPtr : blocks) {
        BasicBlock* bb = bbPtr.get();
        std::vector<std::unique_ptr<Instruction>> newInsts;
        std::vector<std::unique_ptr<Instruction>> phiSpills;
        
        for (auto& instPtr : bb->instructions) {
            Instruction* inst = instPtr.get();
            bool isPhi = (inst->getInstType() == Instruction::Type::Phi);

            if (!isPhi) {
                // filling operands right before use
                std::unordered_map<Instruction*, Instruction*> spilledFills;
                for (Instruction* op : inst->operands) {
                    if (op && op->allocatedStackSlot != -1) {
                        if (spilledFills.find(op) == spilledFills.end()) {
                            auto fill = std::make_unique<FillInst>(op->allocatedStackSlot);
                            fill->name = getNewName();
                            fill->allocatedReg = -1;
                            spilledFills[op] = fill.get();
                            newInsts.push_back(std::move(fill));
                        }
                    }
                }
                
                std::vector<std::pair<Instruction*, Operand>> reps;
                for (Instruction* op : inst->operands) {
                    if (spilledFills.count(op)) reps.push_back({op, Operand(spilledFills[op])});
                }
                for (auto& r : reps) inst->replaceOperand(r.first, r.second);
                
                for (auto& ps : phiSpills) newInsts.push_back(std::move(ps));
                phiSpills.clear();
            }

            bool needsSpill = (inst->allocatedStackSlot != -1 && inst->getInstType() != Instruction::Type::Terminator);
            newInsts.push_back(std::move(instPtr));
            
            if (needsSpill) {
                auto spill = std::make_unique<SpillInst>(Operand(inst), inst->allocatedStackSlot);
                spill->name = getNewName();
                if (isPhi) {
                    phiSpills.push_back(std::move(spill));
                } else {
                    newInsts.push_back(std::move(spill));
                }
            }
        }
        
        for (auto& ps : phiSpills) newInsts.push_back(std::move(ps));
        bb->instructions = std::move(newInsts);
    }
    
    for (auto& bbPtr : blocks) {
        for (auto& instPtr : bbPtr->instructions) {
            if (auto phi = dynamic_cast<PhiInst*>(instPtr.get())) {
                for (auto& inc : phi->incomings) {
                    if (inc.val.type == Operand::Inst && inc.val.instVal->allocatedStackSlot != -1) {
                        BasicBlock* predBB = blockMap[inc.blockName];
                        auto fill = std::make_unique<FillInst>(inc.val.instVal->allocatedStackSlot);
                        fill->name = getNewName();
                        fill->allocatedReg = -1;
                        
                        Instruction* fillRaw = fill.get();
                        
                        auto it = predBB->instructions.end();
                        if (predBB->hasTerminator()) --it;
                        predBB->instructions.insert(it, std::move(fill));
                        
                        auto opIt = std::find(phi->operands.begin(), phi->operands.end(), inc.val.instVal);
                        if (opIt != phi->operands.end()) {
                            *opIt = fillRaw;
                            fillRaw->users.push_back(phi);
                        }
                        inc.val = Operand(fillRaw);
                    }
                }
            }
        }
    }
}