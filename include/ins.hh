#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <stdexcept>

class Instruction;

class Instruction {
public:
    enum class Type { Regular, Terminator, Phi };
    virtual ~Instruction() = default;
    virtual Type getInstType() const { return Type::Regular; }
    virtual void print() const = 0;

    virtual std::vector<std::string> getSuccessorLabels() const { return {}; }
};

class RegularInst : public Instruction {
    std::string repr;
public:
    RegularInst(const std::string& s) : repr(s) {}
    void print() const override { std::cout << "  " << repr << "\n"; }
};

class TerminatorInst : public Instruction {
    std::string repr;
    std::vector<std::string> succLabels;

public:
    TerminatorInst(const std::string& s, const std::vector<std::string> labels)
        : repr(s), succLabels(std::move(labels)) {}

    Type getInstType() const override { return Type::Terminator; }
    void print() const override { std::cout << "  " << repr << "  ; <terminator>\n"; }

    std::vector<std::string> getSuccessorLabels() const override {
        return succLabels;
    }
};

class PhiInst : public Instruction {
public:
    struct Incoming {
        std::string value;
        std::string blockName;
    };
    std::vector<Incoming> incomings;

    void addIncoming(const std::string& val, const std::string& block) {
        incomings.push_back({val, block});
    }

    Type getInstType() const override { return Type::Phi; }

    void print() const override {
        std::cout << "  %phi = phi i32";
        for (size_t i = 0; i < incomings.size(); ++i) {
            if (i == 0) std::cout << " [ " << incomings[i].value << ", %" << incomings[i].blockName << " ]";
            else         std::cout << ", [ " << incomings[i].value << ", %" << incomings[i].blockName << " ]";
        }
        std::cout << "\n";
    }
};
