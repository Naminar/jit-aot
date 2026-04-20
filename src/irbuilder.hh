
#pragma once

#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <vector>
#include <algorithm>

#include "ins.hh"

class Loop;

struct LiveRange {
    int start, end;
    bool operator<(const LiveRange& other) const { return start < other.start; }
};

struct LiveInterval {
    Instruction* reg = nullptr;
    std::vector<LiveRange> ranges;

    void addRange(int start, int end) {
        ranges.push_back({start, end});
        std::sort(ranges.begin(), ranges.end());
        std::vector<LiveRange> merged;
        for (auto& r : ranges) {
            if (merged.empty()) {
                merged.push_back(r);
            } else {
                auto& last = merged.back();
                if (r.start <= last.end) {
                    last.end = std::max(last.end, r.end);
                } else {
                    merged.push_back(r);
                }
            }
        }
        ranges = std::move(merged);
    }

     void setFrom(int start) {
        if (!ranges.empty()) {
            ranges[0].start = start;
        } else {
            ranges.push_back({start, start + 2});
        }
    }
};

class BasicBlock {
public:
    std::string name;
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::vector<BasicBlock*> successors;
    std::vector<BasicBlock*> predecessors;

    int from = -1;
    int to = -1;
    std::unordered_set<Instruction*> liveIn;

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
};

class IRBuilder {
private:
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    std::unordered_map<std::string, BasicBlock*> blockMap;
    BasicBlock* currentBlock = nullptr;
    size_t nextValueID = 0;

    std::vector<BasicBlock*> linearOrder;
    std::unordered_map<Instruction*, LiveInterval> intervals;

public:
    std::string functionName = "main";
    std::vector<std::unique_ptr<Instruction>> params;

    Instruction* createParam() {
        auto inst = std::make_unique<ParamInst>(params.size());
        inst->name = "%p" + std::to_string(params.size());
        Instruction* ptr = inst.get();
        params.push_back(std::move(inst));
        return ptr;
    }

    Instruction* createCall(const std::string& funcName, IRBuilder* targetFunc, const std::vector<Operand>& args) {
        ensureNoTerminator();
        auto inst = std::make_unique<CallInst>(funcName, targetFunc, args);
        inst->name = getNewName();
        Instruction* ptr = inst.get();
        currentBlock->instructions.push_back(std::move(inst));
        return ptr;
    }

    void inlineCall(Instruction* callInst);
    void allocateRegisters(int numRegs);
    void insertSpillFillInstructions();

    LiveInterval* getLiveInterval(Instruction* inst) {
        auto it = intervals.find(inst);
        return it != intervals.end() ? &it->second : nullptr;
    }

    std::vector<LiveRange> getLiveRanges(Instruction* inst) {
        auto it = intervals.find(inst);
        return it != intervals.end() ? it->second.ranges : std::vector<LiveRange>{};
    }

    const std::vector<BasicBlock*>& getLinearOrder() const { return linearOrder; }

    void computeLinearOrder();
    void computeLiveness();

    void printLinearOrder();
    void printLiveness();

private:
    std::unordered_map<BasicBlock*, std::unordered_set<BasicBlock*>> dominates;

    void ensureNoTerminator();

    std::string getNewName() {
        return "%v" + std::to_string(nextValueID++);
    }

    void replaceInstruction(Instruction* oldInst, Operand newVal);
    Operand foldInstruction(Instruction* inst);
    Operand peepholeInstruction(Instruction* inst);

public:

    friend class CallInst; 

    std::vector<Loop*> loopList;
    std::vector<std::unique_ptr<Loop>> allLoops;
    
    BasicBlock* createBasicBlock(const std::string& name);
    void setInsertPoint(BasicBlock* bb) { currentBlock = bb; }

    Instruction* createAdd(Operand lhs, Operand rhs);

    Instruction* createSub(Operand lhs, Operand rhs);

    Instruction* createMul(Operand lhs, Operand rhs);

    Instruction* createAnd(Operand lhs, Operand rhs);

    Instruction* createAShr(Operand lhs, Operand rhs);

    Instruction* createICmp(ICmpInst::Pred pred, Operand lhs, Operand rhs);

    Instruction* createAlloca();

    Instruction* createLoad(Operand ptr);

    void createStore(Operand val, Operand ptr);

    void createNullCheck(Operand ptr);

    void createBoundsCheck(Operand ptr, Operand idx);

    void createBr(Operand cond, const std::string& thenLabel, const std::string& elseLabel);

    void createBr(const std::string& targetLabel);

    void createRet(Operand val);

    void createRet();

    PhiInst* createPHI();

    void buildCFG();

    void dfsVisit(BasicBlock* node, std::unordered_set<BasicBlock*>& visited, BasicBlock* skip = nullptr);
    
    void dump() const {
        for (const auto& bb : blocks) {
            bb->print();
        }
    }

    void computeDominators();

    void computeRPO(std::vector<BasicBlock*>& outRPO);

    void analyzeLoops();

    void collectBackEdges( std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> &backEdgesByHeader, 
        std::unordered_map<BasicBlock*, bool> &isIrreducibleFlag);

    std::unordered_map<std::string, std::unordered_set<std::string>> printDominators();

    void globalOptimization();

    void optimizeChecks();
};