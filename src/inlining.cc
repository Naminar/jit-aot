
#include "irbuilder.hh"
#include <algorithm>

void IRBuilder::inlineCall(Instruction* callInst) {
    CallInst* call = dynamic_cast<CallInst*>(callInst);
    if (!call || !call->targetFunc) return;

    IRBuilder* callee = static_cast<IRBuilder*>(call->targetFunc);
    if (callee->blocks.empty()) return;

    BasicBlock* call_block = nullptr;
    auto call_it = blocks.front()->instructions.end(); 
    
    for (auto& bbPtr : blocks) {
        auto it = std::find_if(bbPtr->instructions.begin(), bbPtr->instructions.end(),
                               [&](const std::unique_ptr<Instruction>& inst) {
                                   return inst.get() == call;
                               });
        if (it != bbPtr->instructions.end()) {
            call_block = bbPtr.get();
            call_it = it;
            break;
        }
    }
    if (!call_block) return;

    // 1. Split block with call instruction into two
    std::string contName = call_block->name + "_cont";
    auto cont_bb_ptr = std::make_unique<BasicBlock>(contName);
    BasicBlock* call_cont_block = cont_bb_ptr.get();

    auto next_it = std::next(call_it);
    while (next_it != call_block->instructions.end()) {
        call_cont_block->instructions.push_back(std::move(*next_it));
        next_it = call_block->instructions.erase(next_it);
    }

    std::unique_ptr<Instruction> removed_call = std::move(*call_it);
    call_block->instructions.erase(call_it);

    call_cont_block->successors = call_block->successors;
    for (auto* succ : call_cont_block->successors) {
        std::replace(succ->predecessors.begin(), succ->predecessors.end(), call_block, call_cont_block);
    }
    call_block->successors.clear();

    BasicBlock* callee_start = callee->blocks.front().get();

    std::unordered_map<std::string, std::string> blockNameMap;
    for (auto& bb : callee->blocks) {
        std::string oldName = bb->name;
        if (bb.get() == callee_start) {
            blockNameMap[oldName] = call_block->name; 
        } else {
            std::string newName = oldName + "_inlined_" + std::to_string(nextValueID++);
            blockNameMap[oldName] = newName;
            bb->name = newName; 
        }
    }

    // 2. Move parameter users to caller input
    for (size_t i = 0; i < callee->params.size(); ++i) {
        Instruction* paramInst = callee->params[i].get();
        Operand argVal = call->args[i];

        std::vector<Instruction*> users = paramInst->users;
        for (Instruction* user : users) {
            user->replaceOperand(paramInst, argVal);
        }
    }

    std::vector<std::pair<Operand, std::string>> returns;

    // 3. Move constants/instructions
    for (auto& bb : callee->blocks) {
        if (bb->hasTerminator()) {
            auto term = static_cast<TerminatorInst*>(bb->instructions.back().get());
            if (term->type == TerminatorInst::Ret) {
                returns.push_back({term->val, bb->name});
                
                bb->instructions.pop_back(); 
                bb->instructions.push_back(std::make_unique<TerminatorInst>(
                    TerminatorInst::Br, Operand(), std::vector<std::string>{call_cont_block->name}));
                bb->successors.push_back(call_cont_block);
                call_cont_block->predecessors.push_back(bb.get());
            } else {
                for (auto& lbl : term->succLabels) {
                    if (blockNameMap.count(lbl)) lbl = blockNameMap[lbl];
                }
            }
        }
        
        for (auto& inst : bb->instructions) {
            inst->name = getNewName(); 
            if (auto phi = dynamic_cast<PhiInst*>(inst.get())) {
                for (auto& inc : phi->incomings) {
                    if (blockNameMap.count(inc.blockName)) {
                        inc.blockName = blockNameMap[inc.blockName];
                    }
                }
            }
        }

        std::replace(bb->successors.begin(), bb->successors.end(), callee_start, call_block);
        std::replace(bb->predecessors.begin(), bb->predecessors.end(), callee_start, call_block);
    }

    for (auto& inst : callee_start->instructions) {
        call_block->instructions.push_back(std::move(inst));
    }
    call_block->successors = callee_start->successors;
    for (auto* succ : call_block->successors) {
        std::replace(succ->predecessors.begin(), succ->predecessors.end(), callee_start, call_block);
    }
    for (auto* pred : callee_start->predecessors) {
        call_block->predecessors.push_back(pred);
    }

    // 4. Update DataFlow for return and generate caller Phi user values
    Operand retVal;
    if (returns.size() == 1) {
        retVal = returns[0].first;
    } else if (returns.size() > 1) {
        auto phi = std::make_unique<PhiInst>();
        phi->name = getNewName();
        for (auto& ret : returns) {
            std::string incName = ret.second;
            if (incName == blockNameMap[callee_start->name]) incName = call_block->name;
            phi->addIncoming(ret.first, incName);
        }
        retVal = Operand(phi.get());
        call_cont_block->instructions.insert(call_cont_block->instructions.begin(), std::move(phi));
    }

    for (Instruction* user : removed_call->users) {
        user->replaceOperand(removed_call.get(), retVal);
    }

    // 5 & 6. Move callee blocks (exclude start natively) to caller graph
    for (auto& bb : callee->blocks) {
        if (bb.get() != callee_start) {
            blockMap[bb->name] = bb.get();
            blocks.push_back(std::move(bb));
        }
    }
    
    blocks.push_back(std::move(cont_bb_ptr));
    blockMap[contName] = call_cont_block;

    callee->blocks.clear();
    callee->blockMap.clear();

    // 7. Remove unreachable blocks
    if (call_cont_block->predecessors.empty()) {
        blocks.erase(std::remove_if(blocks.begin(), blocks.end(),
            [&](const std::unique_ptr<BasicBlock>& bb) { return bb.get() == call_cont_block; }), blocks.end());
        blockMap.erase(call_cont_block->name);
    }
}