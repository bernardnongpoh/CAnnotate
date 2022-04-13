//==============================================================================
// FILE:
//    CAnnotate.cpp
//
// DESCRIPTION:
//    Counts the number of C++ record declarations in the input translation
//    unit. The results are printed on a file-by-file basis (i.e. for each
//    included header file separately).
//
//    Internally, this implementation leverages llvm::StringMap to map file
//    names to the corresponding #count of declarations.
//
// USAGE:
//   clang -cc1 -load <BUILD_DIR>/lib/libCAnnotate.dylib '\'
//    -plugin hello-world test/CAnnotate-basic.cpp
//
// License: The Unlicense
//==============================================================================
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/FileManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;

//-----------------------------------------------------------------------------
// RecursiveASTVisitor
//-----------------------------------------------------------------------------
class CAnnotate : public RecursiveASTVisitor<CAnnotate> {
public:
  explicit CAnnotate(ASTContext *Context, std::shared_ptr<Rewriter> reWriter) : Context(Context),rewriter(reWriter) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *Decl);
  bool VisitIfStmt(IfStmt *ifStmt);

  llvm::StringMap<unsigned> getDeclMap() { return DeclMap; }
  
  std::string stmtToString(Stmt *stmt) {
    clang::LangOptions lo;
    std::string out_str;
    llvm::raw_string_ostream outstream(out_str);
    stmt->printPretty(outstream, NULL, PrintingPolicy(lo));
    return out_str;
  }

private:
  ASTContext *Context;
  std::shared_ptr<Rewriter> rewriter;
  // Map that contains the count of declaration in every input file
  llvm::StringMap<unsigned> DeclMap;
};


bool CAnnotate::VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
  FullSourceLoc FullLocation = Context->getFullLoc(Declaration->getBeginLoc());

  // Basic sanity checking
  if (!FullLocation.isValid())
    return true;

 

  return true;
}

bool CAnnotate::VisitIfStmt(IfStmt *ifStmt){
    

      
      Expr *expr=ifStmt->getCond();
      if(isa<BinaryOperator>(expr)){
        BinaryOperator *binOp=dyn_cast<BinaryOperator>(expr);
        binOp->dumpColor();
        binOp->setOpcode(BinaryOperatorKind::BO_Cmp);
        
      }
      rewriter->ReplaceText(ifStmt->getSourceRange(), stmtToString(ifStmt));
      return true;
}

//-----------------------------------------------------------------------------
// ASTConsumer
//-----------------------------------------------------------------------------
class CAnnotateASTConsumer : public clang::ASTConsumer {
public:
  explicit CAnnotateASTConsumer(ASTContext *Ctx,std::shared_ptr<Rewriter> rewriter) : Visitor(Ctx,rewriter) {}

  void HandleTranslationUnit(clang::ASTContext &Ctx) override {

    Visitor.TraverseDecl(Ctx.getTranslationUnitDecl());
    

    if (Visitor.getDeclMap().empty()) {
      llvm::outs() << "(clang-tutor)  no declarations found "
                   << "\n";
      return;
    }

    for (auto &Element : Visitor.getDeclMap()) {
      llvm::outs() << "(clang-tutor)  file: " << Element.first() << "\n";
      llvm::outs() << "(clang-tutor)  count: " << Element.second << "\n";
    }
  }

private:
  CAnnotate Visitor;
};

//-----------------------------------------------------------------------------
// FrontendAction for CAnnotate
//-----------------------------------------------------------------------------
class FindNamedClassAction : public clang::PluginASTAction {
public:
    FindNamedClassAction():CannotateRewriter{std::make_shared<Rewriter>()} {}
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef InFile) override {
                      CannotateRewriter->setSourceMgr(Compiler.getSourceManager(),
                                        Compiler.getLangOpts());
    return std::unique_ptr<clang::ASTConsumer>(
        std::make_unique<CAnnotateASTConsumer>(&Compiler.getASTContext(),CannotateRewriter));
  }
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }

   void EndSourceFileAction() override {
    // Output to stdout (via llvm::outs()
    CannotateRewriter
        ->getEditBuffer(CannotateRewriter->getSourceMgr().getMainFileID())
        .write(llvm::outs());
  }

  private:
  std::shared_ptr<Rewriter> CannotateRewriter;
};

//-----------------------------------------------------------------------------
// Registration
//-----------------------------------------------------------------------------
static FrontendPluginRegistry::Add<FindNamedClassAction>
    X(/*Name=*/"cannotate", /*Description=*/"The CAnnotate plugin");
