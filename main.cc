
#include "irbuilder.hh"

int main() {
    IRBuilder builder;

    auto* entry = builder.createBasicBlock("entry");
    auto x = builder.createAlloca();
    builder.createStore("42", x);
    auto val = builder.createLoad(x);
    auto cmp = builder.createICmp(ICmpInst::SGT, val, "0");
    builder.createBr(cmp, "then", "else");

    auto* thenBB = builder.createBasicBlock("then");
    auto a = builder.createAdd(val, "1");
    builder.createBr("merge");

    auto* elseBB = builder.createBasicBlock("else");
    auto b = builder.createSub(val, "1");
    builder.createBr("merge");

    auto* mergeBB = builder.createBasicBlock("merge");
    auto* phi = builder.createPHI();
    phi->addIncoming(a, "then");
    phi->addIncoming(b, "else");
    builder.createRet(phi->name);

    builder.dump();

    return 0;
}