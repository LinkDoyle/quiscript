#include <llvm/ADT/STLExtras.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Mangler.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <algorithm>
#include <any>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>

llvm::Value* logErrorV(const std::string& message) {
  std::cerr << message << std::endl;
  return nullptr;
}

class ExprAstNode;
class NumberAstNode;
class VariableExprAstNode;
class BinaryExprAst;
class CallExprAstNode;
class PrototypeAstNode;
class FunctionAstNode;

class AstNodeVisitor {
 public:
  virtual std::any visit(ExprAstNode* node) = 0;
  virtual std::any visit(NumberAstNode* node) = 0;
  virtual std::any visit(VariableExprAstNode* node) = 0;
  virtual std::any visit(BinaryExprAst* node) = 0;
  virtual std::any visit(CallExprAstNode* node) = 0;
  virtual std::any visit(PrototypeAstNode* node) = 0;
  virtual std::any visit(FunctionAstNode* node) = 0;
};

class ExprAstNode {
 public:
  ExprAstNode() = default;
  virtual ~ExprAstNode() {}
  virtual std::any accpet(AstNodeVisitor* visitor) = 0;
};

class NumberAstNode : public ExprAstNode {
 public:
  explicit NumberAstNode(float value) : value_(value) {}
  virtual std::any accpet(AstNodeVisitor* visitor) override {
    return visitor->visit(this);
  }
  double value() const { return value_; }

 private:
  double value_;
};

class VariableExprAstNode : public ExprAstNode {
 public:
  VariableExprAstNode(const std::string& name) : name_(name) {}
  virtual std::any accpet(AstNodeVisitor* visitor) override {
    return visitor->visit(this);
  }
  const std::string& name() const { return name_; }

 private:
  std::string name_;
};

class BinaryExprAst : public ExprAstNode {
 public:
  BinaryExprAst(char op, std::unique_ptr<ExprAstNode> lhs,
                std::unique_ptr<ExprAstNode> rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}
  virtual std::any accpet(AstNodeVisitor* visitor) override {
    return visitor->visit(this);
  }
  ExprAstNode* lhs() const { return lhs_.get(); }
  ExprAstNode* rhs() const { return rhs_.get(); }
  char op() const { return op_; }

 private:
  char op_;
  std::unique_ptr<ExprAstNode> lhs_, rhs_;
};

class CallExprAstNode : public ExprAstNode {
 public:
  CallExprAstNode(const std::string& callee,
                  std::vector<std::unique_ptr<ExprAstNode>> args)
      : callee_(callee), args_(std::move(args)) {}
  virtual std::any accpet(AstNodeVisitor* visitor) override {
    return visitor->visit(this);
  }
  const std::string& callee() const { return callee_; }
  std::vector<ExprAstNode*> args() const {
    std::vector<ExprAstNode*> result;
    std::transform(args_.cbegin(), args_.cend(), result.begin(),
                   [](const auto& arg) { return arg.get(); });
    return result;
  }

 private:
  std::string callee_;
  std::vector<std::unique_ptr<ExprAstNode>> args_;
};

class PrototypeAstNode : public ExprAstNode {
 public:
  PrototypeAstNode(const std::string& Name, std::vector<std::string> Args)
      : name_(Name), args_(std::move(Args)) {}
  virtual std::any accpet(AstNodeVisitor* visitor) override {
    return visitor->visit(this);
  }

  const std::string& name() const { return name_; }
  const std::vector<std::string>& args() const { return args_; }

 private:
  std::string name_;
  std::vector<std::string> args_;
};

class FunctionAstNode : public ExprAstNode {
 public:
  FunctionAstNode(std::unique_ptr<PrototypeAstNode> proto,
                  std::unique_ptr<ExprAstNode> body)
      : proto_(std::move(proto_)), body_(std::move(body)) {}

  virtual std::any accpet(AstNodeVisitor* visitor) override {
    return visitor->visit(this);
  }
  PrototypeAstNode* proto() const { return proto_.get(); }
  ExprAstNode* body() const { return body_.get(); }

 private:
  std::unique_ptr<PrototypeAstNode> proto_;
  std::unique_ptr<ExprAstNode> body_;
};

class CodeGenerator : public AstNodeVisitor {
 public:
  CodeGenerator()
      : builder_(context_),
        module_(std::make_unique<llvm::Module>("my cool jit", context_)) {}

  virtual std::any visit(ExprAstNode* node) override {
    return node->accpet(this);
  }

  virtual std::any visit(NumberAstNode* node) override {
    return llvm::ConstantFP::get(context_, llvm::APFloat(node->value()));
  }

  virtual std::any visit(VariableExprAstNode* node) override {
    llvm::Value* value = nameValues_[node->name()];
    if (!value) {
      return logErrorV("Unknown variable name.");
    }
    return value;
  }

  virtual std::any visit(BinaryExprAst* node) override {
    auto left = std::any_cast<llvm::ConstantFP*>(visit(node->lhs()));
    auto right = std::any_cast<llvm::ConstantFP*>(visit(node->rhs()));
    if (!left || !right) {
      return nullptr;
    }
    switch (node->op()) {
      case '+': {
        return builder_.CreateFAdd(left, right, "addtmp");
      }
      case '-': {
        return builder_.CreateFSub(left, right, "subtmp");
      }
      case '*': {
        return builder_.CreateFMul(left, right, "multmp");
      }
      case '<': {
        auto result = builder_.CreateFCmpULT(left, right, "cmptmp");
        return builder_.CreateUIToFP(result, llvm::Type::getDoubleTy(context_),
                                     "booltmp");
      }
      default: { return logErrorV("invalid binary operator"); }
    }
  }

  virtual std::any visit(CallExprAstNode* node) override {
    auto calleeF = module_->getFunction(node->callee());
    if (!calleeF) {
      return logErrorV("Unknown function referenced");
    }
    auto args = node->args();
    if (calleeF->arg_size() != node->args().size()) {
      return logErrorV("Incorrect # arguments passed");
    }

    std::vector<llvm::Value*> values;
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
      auto value = std::any_cast<llvm::Value*>(visit(args[i]));
      if (!value) {
        return nullptr;
      }
    }

    return builder_.CreateCall(calleeF, values, "calltmp");
  }

  virtual std::any visit(PrototypeAstNode* node) override {
    auto args = node->args();
    std::vector<llvm::Type*> types(args.size(),
                                   llvm::Type::getDoubleTy(context_));
    llvm::FunctionType* functionType = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(context_), types, false);
    llvm::Function* function =
        llvm::Function::Create(functionType, llvm::Function::ExternalLinkage,
                               node->name(), module_.get());
    unsigned idx = 0;
    for (auto& arg : function->args()) {
      arg.setName(args[idx++]);
    }
    return function;
  }

  virtual std::any visit(FunctionAstNode* node) override {
    auto proto = node->proto();

    // First, check for an existing function from a previous 'extern'
    // declaration.
    llvm::Function* function = module_->getFunction(proto->name());
    if (!function) {
      function = std::any_cast<llvm::Function*>(visit(proto));
    }
    if (!function) {
      return nullptr;
    }
    if (!function->empty()) {
      return static_cast<llvm::Function*>(
          logErrorV("Function cannot be redefined."));
    }
    auto BB = llvm::BasicBlock::Create(context_, "entry", function);
    builder_.SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    nameValues_.clear();
    for (auto& arg : function->args()) {
      nameValues_[arg.getName()] = &arg;
    }

    if (auto retValue = std::any_cast<llvm::Value*>(visit(node->body()))) {
      // Finish off the function.
      builder_.CreateRet(retValue);
      // Validate the generated code, checking for consistency.
      llvm::verifyFunction(*function);
      return function;
    }

    // Error reading body, remove function.
    function->eraseFromParent();
    return nullptr;
  }

 private:
  llvm::LLVMContext context_;
  llvm::IRBuilder<> builder_;
  std::unique_ptr<llvm::Module> module_;
  std::unordered_map<std::string, llvm::Value*> nameValues_;
};

int main() {
  auto numberNodeA = std::make_unique<NumberAstNode>(4);
  auto numberNodeB = std::make_unique<NumberAstNode>(2);
  auto binaryExpr = std::make_unique<BinaryExprAst>('+', std::move(numberNodeA),
                                                    std::move(numberNodeB));

  CodeGenerator codeGenerator;
  auto code =
      std::any_cast<llvm::Value*>(codeGenerator.visit(binaryExpr.get()));
  code->print(llvm::errs());

  return 0;
}
