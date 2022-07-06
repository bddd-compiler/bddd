#include "ast.h"
#include <cassert>
#include <iostream>
#include <map>
#include <stack>

SymbolTable symbolTable;

std::stack<std::pair<std::string, std::string>> whileScopes;
FuncDefAST* funcDef = nullptr;

ExprAST::~ExprAST() {
    delete lhs;
    delete rhs;
    delete funcCall;
    delete lval;
}

int labelCnt = 0;
std::string getLabel() {
    return "L" + std::to_string(labelCnt++);
}

int tempCnt = 0;
std::string getTemp() {
    return "temp" + std::to_string(tempCnt++);
}

std::map<Op, std::string> mp {
    {Op::POSITIVE, "+"},
    {Op::NEGATIVE, "-"},
    {Op::PLUS, "+"},
    {Op::MINUS, "-"},
    {Op::MULT, "*"},
    {Op::DIV, "/"},
    {Op::MOD, "%"},
    {Op::LE, "<"},
    {Op::LEQ, "<="},
    {Op::GE, ">"},
    {Op::GEQ, ">="},
    {Op::EQ, "=="},
    {Op::NEQ, "!="},
    {Op::AND, "&&"},
    {Op::OR, "||"},
    {Op::NOT, "!"}
};

void emitQuadruple(std::string op, std::string src1, std::string src2, std::string dest) {
    std::cout << "{" << op << ", " << src1 << ", " << src2 << ", " << dest << "}" << std::endl;
}

void emitLabel(std::string label) {
    std::cout << label << ":" << std::endl;
}

void emitComment(std::string comment) {
    std::cout << "/* " << comment << " */" << std::endl;
}

void CompUnit::appendDecls(vector<DeclAST*> decls) {
    nodes.insert(std::end(nodes), std::begin(decls), std::end(decls));
}

void CompUnit::appendFuncDef(FuncDefAST* funcDef) {
    nodes.emplace_back(funcDef);
}

std::string LVal::codegen() {
    return std::string("LVal");
}

std::string DeclAST::codegen() {
    emitQuadruple("var", (type == VarType::INT ? "int" : "float"), "-", varName);
    if (rhs) {
        auto rhsRes = rhs->codegen();
        emitQuadruple(":=", rhsRes, "-", varName);
    }
    return varName;
}

std::string ExprAST::codegen() {
    auto temp = getTemp();
    switch (op) {
    case Op::FuncCall:
        // TODO: no implementation
        break;
    case Op::CONST_INT:
        emitQuadruple(":=", std::to_string(intVal), "-", temp);
        break;
    case Op::CONST_FLOAT:
        emitQuadruple(":=", std::to_string(floatVal), "-", temp);
        break;
    case Op::LVAL:
        emitQuadruple(":=", lval->name, "-", temp);
        break;
    default:
        // + - * / 
        if (mp.find(op) != mp.end()) {
            auto lhsRes = lhs->codegen();
            auto rhsRes = rhs->codegen();
            emitQuadruple(mp[op], lhsRes, rhsRes, temp);
        } else assert(false);
        break;
    }
    return temp;
}

std::string Cond::codegen() {
    return expr->codegen();
}

std::string BlockAST::codegen() {
    for (auto node : nodes) {
        node->codegen();
    }
    return std::string("BlockAST");
}

std::string FuncDefAST::codegen() {
    emitComment("func " + funcName + " begin");
    block->codegen();
    emitComment("func " + funcName + " end");
    return std::string("FuncDefAST");
}

std::string AssignStmtAST::codegen() {
    auto rhsRes = rhs->codegen();
    emitQuadruple(":=", rhsRes, "-", lval->name);
    return std::string("AssignStmtAST");
}

std::string IfStmtAST::codegen() {
    auto condRes = cond->codegen();

    if (_else) {
        auto ifElseLabel = getLabel();
        auto ifEndLabel = getLabel();
        emitQuadruple("jz", condRes, "-", ifElseLabel);
        _then->codegen();
        emitQuadruple("jmp", "-", "-", ifEndLabel);
        emitLabel(ifElseLabel);
        _else->codegen();
        emitLabel(ifEndLabel);
    } else {
        auto ifEndLabel = getLabel();
        emitQuadruple("jz", condRes, "-", ifEndLabel);
        _then->codegen();
        emitLabel(ifEndLabel);
    }
    return std::string("IfStmtAST");
}

std::string ReturnStmtAST::codegen() {
    if (ret) {
        auto retRes = ret->codegen();
        emitQuadruple("ret", "-", "-", retRes);
    } else {
        emitQuadruple("ret", "-", "-", "-");
    }
    return std::string("ReturnStmtAST");
}

std::string WhileStmtAST::codegen() {
    auto whileStartLabel = getLabel();
    auto whileEndLabel = getLabel();
    whileScopes.emplace(whileStartLabel, whileEndLabel);
    emitLabel(whileStartLabel);

    auto condRes = cond->codegen();
    emitQuadruple("jz", condRes, "-", whileEndLabel);
    emitQuadruple("jmp", "-", "-", whileStartLabel);
    stmt->codegen();
    emitLabel(whileEndLabel);
    whileScopes.pop();
    return std::string("WhileStmtAST");
}

std::string FuncCall::codegen() {
    // TODO: not considered
    return std::string("FuncCall");
}

std::string FuncFParam::codegen() {
    // TODO: not considered
    return std::string("FuncFParam");
}

std::string EvalStmtAST::codegen() {
    // TODO: not considered
    return std::string("EvalStmtAST");
}

std::string BreakStmtAST::codegen() {
    if (!whileScopes.empty()) {
        emitQuadruple("jmp", "-", "-", whileScopes.top().second);
    }
    return std::string("BreakStmtAST");
}

std::string ContinueStmtAST::codegen() {
    if (!whileScopes.empty()) {
        emitQuadruple("jmp", "-","-", whileScopes.top().first);
    }
    return std::string("ContinueStmtAST");
}

std::string CompUnit::codegen() {
    for (auto node : nodes) {
        node->codegen();
    }
    return std::string("CompUnit");
}


bool SymbolTable::insert(std::string name, ElemType type, std::variant<int, float, FuncDefAST*> val) {
    bool notFound = (intVals.count(name) + floatVals.count(name) + funcDefs.count(name) == 0);
    if (!notFound) return false;

    if (type == ElemType::INT) {
        intVals[name] = std::get<int>(val);
    } else if (type == ElemType::FLOAT) {
        floatVals[name] = std::get<float>(val);
    } else if (type == ElemType::FUNC) {
        funcDefs[name] = std::get<FuncDefAST*>(val);
    } else { return false; }
    return true;
}

bool SymbolTable::remove(std::string name) {
    auto it1 = intVals.find(name);
    if (it1 != intVals.end()) {
        intVals.erase(it1);
        return true;
    }
    auto it2 = floatVals.find(name);
    if (it2 != floatVals.end()) {
        floatVals.erase(it2);
        return true;
    }
    auto it3 = funcDefs.find(name);
    if (it3 != funcDefs.end()) {
        funcDefs.erase(it3);
        return true;
    }
    return false;
}

SymbolTable::ElemType SymbolTable::lookup(std::string name) {
    auto it1 = intVals.find(name);
    if (it1 != intVals.end()) {
        return ElemType::INT;
    }
    auto it2 = floatVals.find(name);
    if (it2 != floatVals.end()) {
        return ElemType::FLOAT;
    }
    auto it3 = funcDefs.find(name);
    if (it3 != funcDefs.end()) {
        return ElemType::FUNC;
    }
    return ElemType::NOT_FOUND;
}

bool SymbolTable::check(std::string name, ElemType type) {
    auto foundType = lookup(name);
    return type == foundType;
}

std::pair<SymbolTable::ElemType, std::variant<int, float>> SymbolTable::getValue(std::string name) {
    auto it1 = intVals.find(name);
    if (it1 != intVals.end()) {
        return std::make_pair(ElemType::INT, it1->second);
    }
    auto it2 = floatVals.find(name);
    if (it2 != floatVals.end()) {
        return std::make_pair(ElemType::FLOAT, it2->second);
    }
    return std::make_pair(ElemType::NOT_FOUND, 0);
}

bool LVal::typecheck() {
    if (symbolTable.lookup(name) != SymbolTable::ElemType::NOT_FOUND) {
        return true;
    } else {
        throw GrammarException("LVal not found in symbol table");
    }
}

std::pair<ExprAST::EvalType, std::variant<int, float, bool>> ExprAST::evaluate() {
    switch (op) {
    case Op::FuncCall:
        // TODO: unimplemented
        return std::make_pair(EvalType::INT, 0);
    case Op::CONST_INT:
        return std::make_pair(EvalType::INT, intVal);
    case Op::CONST_FLOAT:
        return std::make_pair(EvalType::FLOAT, floatVal);
    case Op::LVAL: {
        auto res = symbolTable.getValue(lval->name);
        if (res.first == SymbolTable::ElemType::INT) {
            return std::make_pair(EvalType::INT, std::get<int>(res.second));
        } else if (res.first == SymbolTable::ElemType::FLOAT) {
            return std::make_pair(EvalType::FLOAT, std::get<float>(res.second));
        } else {
            return std::make_pair(EvalType::ERROR, false);
        }
    }
    default: {
        if (mp.find(op) != mp.end()) {
            if (op == Op::POSITIVE) {
                auto lhsRes = lhs->evaluate();
                if (lhsRes.first == EvalType::INT) {
                    return std::make_pair(EvalType::INT, std::get<int>(lhsRes.second));
                } else if (lhsRes.first == EvalType::FLOAT) {
                    return std::make_pair(EvalType::FLOAT, std::get<float>(lhsRes.second));
                } else {
                    throw GrammarException("unary operator positive not follows int or float");
                }
            } else if (op == Op::NEGATIVE) {
                auto lhsRes = lhs->evaluate();
                if (lhsRes.first == EvalType::INT) {
                    return std::make_pair(EvalType::INT, -std::get<int>(lhsRes.second));
                } else if (lhsRes.first == EvalType::FLOAT) {
                    return std::make_pair(EvalType::FLOAT, -std::get<float>(lhsRes.second));
                } else {
                    throw GrammarException("unary operator negative not follows int or float");
                }
            } else if (op == Op::NOT) {
                auto lhsRes = lhs->evaluate();
                if (lhsRes.first == EvalType::BOOL) {
                    return std::make_pair(EvalType::BOOL, !std::get<bool>(lhsRes.second));
                } else {
                    throw GrammarException("unary operator not not follows bool");
                }
            }

            auto lhsRes = lhs->evaluate();
            auto rhsRes = rhs->evaluate();

            if (lhsRes.first == EvalType::INT && rhsRes.first == EvalType::INT) {
                if (op == Op::PLUS)
                    return std::make_pair(EvalType::INT, std::get<int>(lhsRes.second) + std::get<int>(rhsRes.second));
                else if (op == Op::MINUS)
                    return std::make_pair(EvalType::INT, std::get<int>(lhsRes.second) - std::get<int>(rhsRes.second));
                else if (op == Op::MULT)
                    return std::make_pair(EvalType::INT, std::get<int>(lhsRes.second) * std::get<int>(rhsRes.second));
                else if (op == Op::DIV) {
                    auto x = std::get<int>(rhsRes.second);
                    if (x == 0) return std::make_pair(EvalType::ERROR, false);
                    return std::make_pair(EvalType::INT, std::get<int>(lhsRes.second) / x);
                }
                else if (op == Op::MOD) {
                    auto x = std::get<int>(rhsRes.second);
                    if (x == 0) return std::make_pair(EvalType::ERROR, false);
                    return std::make_pair(EvalType::INT, std::get<int>(lhsRes.second) % x);
                }
                else if (op == Op::GE) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) > std::get<int>(rhsRes.second));
                else if (op == Op::GEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) >= std::get<int>(rhsRes.second));
                else if (op == Op::LE) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) < std::get<int>(rhsRes.second));
                else if (op == Op::LEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) <= std::get<int>(rhsRes.second));
                else if (op == Op::EQ) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) == std::get<int>(rhsRes.second));
                else if (op == Op::NEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) != std::get<int>(rhsRes.second));
                else 
                    return std::make_pair(EvalType::ERROR, false);
                
            } else if (lhsRes.first == EvalType::INT && rhsRes.first == EvalType::FLOAT) {
                if (op == Op::PLUS)
                    return std::make_pair(EvalType::INT, std::get<int>(lhsRes.second) + std::get<float>(rhsRes.second));
                else if (op == Op::MINUS)
                    return std::make_pair(EvalType::INT, std::get<int>(lhsRes.second) - std::get<float>(rhsRes.second));
                else if (op == Op::MULT)
                    return std::make_pair(EvalType::INT, std::get<int>(lhsRes.second) * std::get<float>(rhsRes.second));
                else if (op == Op::DIV) {
                    auto x = std::get<float>(rhsRes.second);
                    if (x == 0) return std::make_pair(EvalType::ERROR, false);
                    return std::make_pair(EvalType::INT, std::get<int>(lhsRes.second) / x);
                }
                else if (op == Op::GE) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) > std::get<float>(rhsRes.second));
                else if (op == Op::GEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) >= std::get<float>(rhsRes.second));
                else if (op == Op::LE) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) < std::get<float>(rhsRes.second));
                else if (op == Op::LEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) <= std::get<float>(rhsRes.second));
                else if (op == Op::EQ) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) == std::get<float>(rhsRes.second));
                else if (op == Op::NEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<int>(lhsRes.second) != std::get<float>(rhsRes.second));
                else 
                    return std::make_pair(EvalType::ERROR, false);
            } else if (lhsRes.first == EvalType::FLOAT && rhsRes.first == EvalType::INT) {
                if (op == Op::PLUS)
                    return std::make_pair(EvalType::FLOAT, std::get<float>(lhsRes.second) + std::get<int>(rhsRes.second));
                else if (op == Op::MINUS)
                    return std::make_pair(EvalType::FLOAT, std::get<float>(lhsRes.second) - std::get<int>(rhsRes.second));
                else if (op == Op::MULT)
                    return std::make_pair(EvalType::FLOAT, std::get<float>(lhsRes.second) * std::get<int>(rhsRes.second));
                else if (op == Op::DIV) {
                    auto x = std::get<int>(rhsRes.second);
                    if (x == 0) return std::make_pair(EvalType::ERROR, false);
                    return std::make_pair(EvalType::FLOAT, std::get<float>(lhsRes.second) / x);
                }
                else if (op == Op::GE) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) > std::get<int>(rhsRes.second));
                else if (op == Op::GEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) >= std::get<int>(rhsRes.second));
                else if (op == Op::LE) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) < std::get<int>(rhsRes.second));
                else if (op == Op::LEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) <= std::get<int>(rhsRes.second));
                else if (op == Op::EQ) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) == std::get<int>(rhsRes.second));
                else if (op == Op::NEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) != std::get<int>(rhsRes.second));
                else 
                    return std::make_pair(EvalType::ERROR, false);
            } else if (lhsRes.first == EvalType::FLOAT && rhsRes.first == EvalType::FLOAT) {
                if (op == Op::PLUS)
                    return std::make_pair(EvalType::FLOAT, std::get<float>(lhsRes.second) + std::get<float>(rhsRes.second));
                else if (op == Op::MINUS)
                    return std::make_pair(EvalType::FLOAT, std::get<float>(lhsRes.second) - std::get<float>(rhsRes.second));
                else if (op == Op::MULT)
                    return std::make_pair(EvalType::FLOAT, std::get<float>(lhsRes.second) * std::get<float>(rhsRes.second));
                else if (op == Op::DIV) {
                    auto x = std::get<float>(rhsRes.second);
                    if (x == 0) return std::make_pair(EvalType::ERROR, false);
                    return std::make_pair(EvalType::FLOAT, std::get<float>(lhsRes.second) / x);
                }
                else if (op == Op::GE) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) > std::get<float>(rhsRes.second));
                else if (op == Op::GEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) >= std::get<float>(rhsRes.second));
                else if (op == Op::LE) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) < std::get<float>(rhsRes.second));
                else if (op == Op::LEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) <= std::get<float>(rhsRes.second));
                else if (op == Op::EQ) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) == std::get<float>(rhsRes.second));
                else if (op == Op::NEQ) 
                    return std::make_pair(EvalType::BOOL, std::get<float>(lhsRes.second) != std::get<float>(rhsRes.second));
                else 
                    return std::make_pair(EvalType::ERROR, false);
            } else if (lhsRes.first == EvalType::BOOL && rhsRes.first == EvalType::BOOL) {
                if (op == Op::AND) 
                    return std::make_pair(EvalType::BOOL, std::get<bool>(lhsRes.second) && std::get<bool>(rhsRes.second));
                else if (op == Op::OR)
                    return std::make_pair(EvalType::BOOL, std::get<bool>(lhsRes.second) || std::get<bool>(rhsRes.second));
                else if (op == Op::NOT)
                    return std::make_pair(EvalType::BOOL, !std::get<bool>(lhsRes.second));
                else 
                    return std::make_pair(EvalType::ERROR, false);
            } else {
                return std::make_pair(EvalType::ERROR, false);
            }
        } else {
            assert(false);
            return std::make_pair(EvalType::ERROR, false);
        }
    }
    }
}

bool ExprAST::typecheck() {
    switch (op) {
    case Op::FuncCall:
        // TODO: no implementation
        return false;
    case Op::CONST_INT:
    case Op::CONST_FLOAT:
        return true;
    case Op::LVAL:
        return lval->typecheck();
    default: {
        if (evaluate().first != EvalType::ERROR) {
            return true;
        } else {
            throw GrammarException("illegal expression with error type");
        }
    }
    }
}

bool DeclAST::typecheck() {
    auto s = symbolTable.lookup(varName);
    if (s == SymbolTable::ElemType::NOT_FOUND) {
        auto a = (type == VarType::INT ? SymbolTable::ElemType::INT : SymbolTable::ElemType::FLOAT);
        auto rhsRes = rhs->evaluate();
        switch (rhsRes.first) {
        case ExprAST::EvalType::INT:
            symbolTable.insert(varName, a, std::get<int>(rhsRes.second));
            break;
        case ExprAST::EvalType::FLOAT:
            symbolTable.insert(varName, a, std::get<float>(rhsRes.second));
            break;
        case ExprAST::EvalType::ERROR:
            throw GrammarException("error type when evaluating rhs");
            break;
        default:
            throw GrammarException("unexpected type found in symbol table");
            break;
        }
        return true;
    }
    throw GrammarException("variable redefined");
    return false;
}

bool FuncCall::typecheck() {
    // TODO: 
    return true;
}

bool Cond::typecheck() {
    auto x = expr->evaluate();
    if (x.first == ExprAST::EvalType::BOOL) {
        return true;
    } else {
        throw GrammarException("evaluated type of cond is not bool");
    }
}

bool FuncFParam::typecheck() {
    // TODO:
    return true;
}

bool BlockAST::typecheck() {
    for (auto node : nodes) {
        if (!node->typecheck()) return false;
    }
    return true;
}

bool FuncDefAST::typecheck() {
    auto s = symbolTable.lookup(funcName);
    if (s == SymbolTable::ElemType::NOT_FOUND) {
        funcDef = this;
        symbolTable.insert(funcName, SymbolTable::ElemType::FUNC, this);
        if (!block->typecheck()) return false;
        funcDef = nullptr;
        return true;
    } else {
        throw GrammarException("function redefined");
        return false;
    }
}

bool AssignStmtAST::typecheck() {
    if (!lval->typecheck()) return false;
    if (!rhs->typecheck()) return false;
    return true;
}

bool EvalStmtAST::typecheck() {
    // TODO
    return true;
}

bool IfStmtAST::typecheck() {
    if (!cond->typecheck()) return false;
    if (!_then->typecheck()) return false;
    if (_else) {
        if (!_else->typecheck()) return false;
    }
    return true;
}

bool ReturnStmtAST::typecheck() {
    if (funcDef != nullptr) {
        if (ret) return ret->typecheck();
        else return true;
    } else {
        throw GrammarException("return statement not in funcDefinition");
    }
}

bool WhileStmtAST::typecheck() {
    if (!cond->typecheck()) return false;
    if (!stmt->typecheck()) return false;
    return true;
}

bool BreakStmtAST::typecheck() {
    // TODO
    return true;
}

bool ContinueStmtAST::typecheck() {
    // TODO
    return true;
}

bool CompUnit::typecheck() {
    for (auto node : nodes) {
        if (!node->typecheck()) return false;
    }
    return true;
}