#include <antlr4-runtime.h>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include "XMLLexer.h"
#include "XMLParser.h"
#include "XMLParserBaseVisitor.h"

using namespace antlr4;
using namespace antlrxmlparser;

class Visitor : public XMLParserBaseVisitor {
 public:
  virtual antlrcpp::Any visitDocument(
      XMLParser::DocumentContext* ctx) override {
    std::cout << ctx->getText() << std::endl;
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitProlog(XMLParser::PrologContext* ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitContent(XMLParser::ContentContext* ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitElement(XMLParser::ElementContext* ctx) override {
    for (auto attribute : ctx->attribute()) {
      auto name = attribute->Name()->getText();
      auto value = attribute->STRING()->getText();
      std::cout << name << ": " << value << "\n";
    }
    std::cout << ctx->getText() << std::endl;
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitReference(
      XMLParser::ReferenceContext* ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAttribute(
      XMLParser::AttributeContext* ctx) override {
    return visitChildren(ctx);
  }
  virtual antlrcpp::Any visitChardata(
      XMLParser::ChardataContext* ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitMisc(XMLParser::MiscContext* ctx) override {
    return visitChildren(ctx);
  }
};

class ErrorListener : public antlr4::BaseErrorListener {
  virtual void syntaxError(Recognizer* recognizer, Token* offendingSymbol,
                           size_t line, size_t charPositionInLine,
                           const std::string& msg,
                           std::exception_ptr e) override {
    auto stack =
        static_cast<antlr4::Parser*>(recognizer)->getRuleInvocationStack();
    std::cout << "Parsing failed!:\n";
    std::cout << "  Rule Stack:\n";
    for (auto info : stack) {
      std::cout << " " << info << "\n";
    }
    std::cout << "  line: " << line << ":" << charPositionInLine << " at "
              << offendingSymbol->toString() << ": " << msg << "\n";
  }
};

int main() {
  ANTLRInputStream input(R"(
    <View>
      <TextBox Content="Hello XML</>~"/>
    </View>
)");

  antlrxmlparser::XMLLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  antlrxmlparser::XMLParser parser(&tokens);
  auto errorListener = std::make_unique<ErrorListener>();
  parser.removeErrorListeners();
  parser.addErrorListener(errorListener.get());
  try {
    tree::ParseTree* tree = parser.document();
    [[maybe_unused]] auto visitor = std::make_unique<Visitor>();
    [[maybe_unused]] auto result = tree->accept(visitor.get());
    std::cout << "Parse Tree: " << tree->toStringTree(&parser) << std::endl;
  } catch (antlr4::LexerNoViableAltException& ex) {
    std::cout << "error: " << ex.toString() << "\n";
  }
  return 0;
}
