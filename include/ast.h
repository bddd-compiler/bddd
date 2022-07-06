#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <map>
#include <ostream>

using std::string;
using std::vector;

enum class Op {
    POSITIVE,
    NEGATIVE,

    PLUS,
    MINUS,
    MULT,
    DIV,
    MOD,

    LE,
    LEQ,
    GE,
    GEQ,
    EQ,
    NEQ,

    AND,
    OR,
    NOT,

    CONST_INT,
    CONST_FLOAT,
    LVAL,
    FuncCall
};

enum class VarType {
    INT,
    FLOAT,
    VOID,
};

class Node {
public:
    virtual ~Node() = default;
    virtual std::string codegen() = 0;
    virtual bool typecheck() = 0;
};

class StmtAST : public Node {};

class LVal : public Node {
public:
    string name;

    explicit LVal(const string& name):
        name(name) {}

    std::string codegen() override;

    bool typecheck() override;
};

class FuncCall;

class ExprAST : public Node {
public:
    enum class EvalType {
        INT,
        FLOAT,
        BOOL,
        ERROR
    };

    Op op;
    ExprAST* lhs;
    ExprAST* rhs;
    FuncCall* funcCall;
    int intVal;
    float floatVal;
    LVal* lval;

    explicit ExprAST(Op op, ExprAST* lhs, ExprAST* rhs = nullptr)
        : op(op), lhs(lhs), rhs(rhs), funcCall(nullptr), intVal(0), floatVal(0.0), lval(nullptr) {}

    explicit ExprAST(FuncCall* funcCall) 
        : op(Op::FuncCall), lhs(nullptr), rhs(nullptr), funcCall(funcCall), intVal(0), floatVal(0.0), lval(nullptr) {}  
    
    explicit ExprAST(int val)
        : op(Op::CONST_INT), lhs(nullptr), rhs(nullptr), funcCall(nullptr), intVal(val), floatVal(0.0), lval(nullptr) {}

    explicit ExprAST(float val)
        : op(Op::CONST_FLOAT), lhs(nullptr), rhs(nullptr), funcCall(nullptr), intVal(0), floatVal(val), lval(nullptr) {}

    explicit ExprAST(LVal* lval)
        : op(Op::LVAL), lhs(nullptr), rhs(nullptr), funcCall(nullptr), intVal(0), floatVal(0.0), lval(lval) {}
        
    ~ExprAST();

    std::pair<EvalType, std::variant<int, float, bool>> evaluate();

    std::string codegen() override;

    bool typecheck() override;
};

class DeclAST : public Node {
public:
    bool isConst;
    VarType type;
    string varName;
    ExprAST* rhs;
    explicit DeclAST(const string& varName, ExprAST* rhs = nullptr):
        isConst(false), type(VarType::INT), varName(varName), rhs(rhs) {}

    ~DeclAST() {
        delete rhs;
    }

    std::string codegen() override;
    
    bool typecheck() override;
};

class FuncCall : public Node {
public:
    string callName;
    vector<ExprAST*> params;

    explicit FuncCall(const string& callName):
        callName(callName), params() {}
    
    ~FuncCall() {
        for (auto it : params) {
            delete it;
        }
    }

    std::string codegen() override;

    bool typecheck() override;
};


class Cond : public Node {
public:
    ExprAST* expr;

    explicit Cond(ExprAST* expr = nullptr):
        expr(expr) {}
    
    ~Cond() {
        delete expr;
    }

    std::string codegen() override;

    bool typecheck() override;
};

class FuncFParam : public Node {
public:
    VarType type;
    string name;

    explicit FuncFParam(VarType type, const string& name):
        type(type), name(name) {}

    std::string codegen() override;

    bool typecheck() override;
};

class BlockAST : public StmtAST {
public:
    vector<Node*> nodes;

    ~BlockAST() {
        for (auto it : nodes) {
            delete it;
        }
    }

    void appendNodes(vector<Node*> _nodes) {
        nodes.insert(std::end(nodes), std::begin(_nodes), std::end(_nodes));
    }

    std::string codegen() override;

    bool typecheck() override;
};

class FuncDefAST : public Node {
public:
    VarType returnType;
    string funcName;
    vector<FuncFParam*> fParams;
    BlockAST* block;

    explicit FuncDefAST(VarType returnType, const string& funcName, vector<FuncFParam*> fParams, BlockAST* block):
        returnType(returnType), funcName(funcName), fParams(fParams), block(block) {}

    explicit FuncDefAST(VarType returnType, const string& funcName, BlockAST* block):
        returnType(returnType), funcName(funcName), fParams(), block(block) {}

    ~FuncDefAST() {
        for (auto it : fParams) {
            delete it;
        }
        delete block;
    }

    std::string codegen() override;

    bool typecheck() override;
};

class AssignStmtAST : public StmtAST {
public:
    LVal* lval;
    ExprAST* rhs;

    explicit AssignStmtAST(LVal* lval, ExprAST* rhs):
        lval(lval), rhs(rhs) {}
    
    ~AssignStmtAST() {
        delete lval;
        delete rhs;
    }

    std::string codegen() override;

    bool typecheck() override;
};

class EvalStmtAST : public StmtAST {
public:
    ExprAST* expr;

    explicit EvalStmtAST(ExprAST* expr = nullptr):
        expr(expr) {}
    
    ~EvalStmtAST() {
        delete expr;
    }

    std::string codegen() override;

    bool typecheck() override;
};

class IfStmtAST : public StmtAST {
public:
    Cond* cond;
    StmtAST* _then;
    StmtAST* _else;

    explicit IfStmtAST(Cond* cond, StmtAST* _then, StmtAST* _else = nullptr):
        cond(cond), _then(_then), _else(_else) {}
    
    ~IfStmtAST() {
        delete cond;
        delete _then;
        delete _else;
    }
    
    std::string codegen() override;

    bool typecheck() override;
};

class ReturnStmtAST : public StmtAST {
public:
    ExprAST* ret;

    explicit ReturnStmtAST(ExprAST* ret = nullptr):
        ret(ret) {}

    ~ReturnStmtAST() {
        delete ret;
    }

    std::string codegen() override;

    bool typecheck() override;
};

class WhileStmtAST : public StmtAST {
public:
    Cond* cond;
    StmtAST* stmt;

    explicit WhileStmtAST(Cond* cond, StmtAST* stmt):
        cond(cond), stmt(stmt) {}
    
    ~WhileStmtAST() {
        delete cond;
        delete stmt;
    }

    std::string codegen() override;

    bool typecheck() override;
};

class BreakStmtAST : public StmtAST {
public:
    std::string codegen() override;
    bool typecheck() override;
};

class ContinueStmtAST : public StmtAST {
public:
    std::string codegen() override;
    bool typecheck() override;
};

class CompUnit : public Node {
public:
    vector<Node*> nodes;
    void appendDecls(vector<DeclAST*> decls);

    void appendFuncDef(FuncDefAST* funcDef);

    std::string codegen() override;
    bool typecheck() override;
};

class SymbolTable {
private:
    std::map<std::string, int> intVals;
    std::map<std::string, float> floatVals;
    std::map<std::string, FuncDefAST*> funcDefs;
public:
    enum class ElemType {
        INT,
        FLOAT,
        FUNC,

        NOT_FOUND,
    };
    ElemType lookup(std::string name);

    bool insert(std::string name, ElemType type, std::variant<int, float, FuncDefAST*> val);

    bool remove(std::string name);

    bool check(std::string name, ElemType type);

    std::pair<ElemType, std::variant<int, float>> getValue(std::string name);
};

class GrammarException : public std::exception {
private:
  std::string msg;
public:
  explicit GrammarException(const std::string& msg) : msg(msg) {}

  const std::string& getMsg() const { return msg; }

  friend std::ostream& operator << (std::ostream& out, GrammarException e) {
    out << "[grammar error] ";
    out << ": " << e.getMsg() << std::endl;
    return out;
  }
};


#endif //AST_H