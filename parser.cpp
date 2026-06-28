#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <sstream>

// ================================================================
// LEXER
// ================================================================

enum Token {
    tok_eof      = -1,
    tok_do       = -2,
    tok_madame   = -3,
    tok_identifier = -4,
    tok_number   = -5
};

static std::string IdentifierStr;
static double NumVal;

// ================================================================
// INPUT BUFFER
// Replaces getchar() so tests can feed strings instead of stdin.
// Call resetParser() before each test to load a new input string.
// ================================================================

static std::string g_inputBuf;
static size_t      g_inputPos = 0;
       int         g_lastChar = ' ';

static int nextChar() {
    if (g_inputPos < g_inputBuf.size())
        return (unsigned char)g_inputBuf[g_inputPos++];
    return EOF;
}

static int gettok() {
    int &LastChar = g_lastChar;   // alias into the resettable global

    while (isspace(LastChar))
        LastChar = nextChar();

    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum((LastChar = nextChar())))
            IdentifierStr += LastChar;

        if (IdentifierStr == "Do")     return tok_do;
        if (IdentifierStr == "Madame") return tok_madame;
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        int decimal_count = 0;
        do {
            NumStr += LastChar;
            LastChar = nextChar();
            if (LastChar == '.') decimal_count++;
        } while (isdigit(LastChar) || (LastChar == '.' && decimal_count <= 1));

        if (decimal_count <= 1) {
            NumVal = strtod(NumStr.c_str(), nullptr);
            return tok_number;
        }
    }

    if (LastChar == '#') {
        do LastChar = nextChar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
        if (LastChar != EOF) return gettok();
    }

    if (LastChar == EOF) return tok_eof;

    int ThisChar = LastChar;
    LastChar = nextChar();
    return ThisChar;
}


class ExprAST {
public:
    virtual ~ExprAST() = default;

    // Returns a human-readable description of this node and its children.
    // indent grows by 2 spaces each level so you can see the tree shape.
    virtual std::string toString(int indent = 0) const = 0;
};

// A literal number like 3.14 or 0
class NumberExprAST : public ExprAST {
    double Val;
public:
    NumberExprAST(double Val) : Val(Val) {}

    std::string toString(int indent = 0) const override {
        return std::string(indent, ' ') + "Number(" + std::to_string(Val) + ")";
    }
};

// A variable reference like `a` or `count`
class VariableExprAST : public ExprAST {
    std::string Name;
public:
    VariableExprAST(const std::string &Name) : Name(Name) {}

    std::string toString(int indent = 0) const override {
        return std::string(indent, ' ') + "Variable(" + Name + ")";
    }
};

// A binary expression like `a + b` or `count < n`
class BinaryExprAST : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;
public:
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

    std::string toString(int indent = 0) const override {
        std::string s = std::string(indent, ' ') + "Binary(";
        s += Op;
        s += ")\n";
        s += LHS->toString(indent + 2) + "\n";
        s += RHS->toString(indent + 2);
        return s;
    }
};

// A function call like `fibonacci(limit)` or `Inscribe(a)`
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;
public:
    CallExprAST(const std::string &Callee,
                std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}

    std::string toString(int indent = 0) const override {
        std::string s = std::string(indent, ' ') + "Call(" + Callee + ")\n";
        for (const auto &arg : Args)
            s += arg->toString(indent + 2) + "\n";
        return s;
    }
};

// A function signature like `fibonacci(n)` — name + parameter list
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;
public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}

    const std::string &getName() const { return Name; }

    std::string toString(int indent = 0) const {
        std::string s = std::string(indent, ' ') + "Prototype(" + Name + "(";
        for (size_t i = 0; i < Args.size(); i++) {
            if (i > 0) s += ", ";
            s += Args[i];
        }
        s += "))";
        return s;
    }
};

// A full function definition: signature + body expression
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;
public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body)
        : Proto(std::move(Proto)), Body(std::move(Body)) {}

    std::string toString(int indent = 0) const {
        std::string s = std::string(indent, ' ') + "Function\n";
        s += Proto->toString(indent + 2) + "\n";
        s += std::string(indent + 2, ' ') + "Body:\n";
        s += Body->toString(indent + 4);
        return s;
    }
};

// ================================================================
// PARSER CORE
// ================================================================

static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

// Forward declaration — ParseExpression calls ParseBinOpRHS which calls
// ParsePrimary which calls ParseParenExpr which calls ParseExpression.
static std::unique_ptr<ExprAST> ParseExpression();

// ParseNumberExpr
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

// ParseParenExpr — parse a parenthesised expression.
static std::unique_ptr<ExprAST> ParseParenExpr() {
    
    if(CurTok != '(') return LogError("Expected '(' in ParseParenExpr");
    getNextToken();
    auto Result = ParseExpression();
    if(CurTok != ')') return LogError("Expected ')' in ParseParenExpr");
    getNextToken();
    return Result;
}

// ParseIdentifierExpr — parse a variable reference OR a function call.
//
// Called when: CurTok == tok_identifier
// It should:
//   1. Save IdentifierStr, advance the token
//   2. If the next token is NOT '(', return a VariableExprAST
//   3. If the next token IS '(':
//        a. Consume the '('
//        b. Parse comma-separated expressions until ')'
//        c. Consume the ')'
//        d. Return a CallExprAST with the collected arguments
//
// Input:  a
// Output: Variable(a)
//
// Input:  fibonacci(limit)
// Output: Call(fibonacci)
//           Variable(limit)
//
// Input:  add(1, 2)
// Output: Call(add)
//           Number(1.000000)
//           Number(2.000000)
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {

    if(CurTok == tok_identifier){
        auto IdName = IdentifierStr;
        getNextToken();
        if(CurTok != '(') {
            return std::make_unique<VariableExprAST>(IdName);
        } else {
            getNextToken();

            std::vector<std::unique_ptr<ExprAST>> Args;

            while(CurTok != ')'){
                auto arg = ParseExpression();
                if(!arg) return nullptr;
                Args.push_back(std::move(arg));
                if (CurTok == ')') break;
                if(CurTok == ',') return LogError("Expected ')'");
                getNextToken();
            }

            getNextToken();
            return std::make_unique<CallExprAST>(IdName, std::move(Args));
        }
    } 
    return LogError("No Identifier");
}

// ParsePrimary — the entry point for any single value or expression atom.
//
// Called when: we need the left-hand side of something, or a standalone value
// It should:   look at CurTok and dispatch to the right parse function:
//                tok_identifier -> ParseIdentifierExpr()
//                tok_number     -> ParseNumberExpr()
//                '('            -> ParseParenExpr()
//                anything else  -> LogError("unknown token")
//
// This one is short — it's just a switch on CurTok.
static std::unique_ptr<ExprAST> ParsePrimary() {
    
    if(CurTok == tok_identifier) return ParseIdentifierExpr();
    if(CurTok == tok_number) return ParseNumberExpr();
    if(CurTok == '(') return ParseParenExpr();
    return LogError("ParsePrimary not implemented yet");
}

// Operator precedence table.
// Higher number = tighter binding.  Add operators here to support them.
static std::map<char, int> BinopPrecedence = {
    {'<', 10},
    {'+', 20},
    {'-', 20},
    {'*', 40},
};

static int GetTokPrecedence() {
    if (!isascii(CurTok)) return -1;
    int TokPrec = BinopPrecedence.count(CurTok) ? BinopPrecedence[CurTok] : -1;
    return TokPrec;
}

// ParseBinOpRHS — parse the right-hand side of a binary expression.
//
// Called by: ParseExpression, with the LHS already parsed
// Parameters:
//   ExprPrec — the minimum precedence level this call is allowed to consume
//   LHS      — the expression we have so far on the left
//
// It should loop as long as the current operator has precedence >= ExprPrec:
//   1. Save the operator, advance the token
//   2. Parse the next primary as RHS
//   3. If the *next* operator has higher precedence than the current one,
//      recursively call ParseBinOpRHS with ExprPrec+1 to grab it first
//      (this is what makes * bind tighter than +)
//   4. Combine LHS and RHS into a BinaryExprAST, repeat
//
// Input:  1 + 2
// Output: Binary(+)
//           Number(1.000000)
//           Number(2.000000)
//
// Input:  a + b * c          <-- * should bind tighter
// Output: Binary(+)
//           Variable(a)
//           Binary(*)
//             Variable(b)
//             Variable(c)
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
    
    while(GetTokPrecedence() >= ExprPrec){
        int BinOp = CurTok;
        getNextToken();
        auto RHS = ParsePrimary();
        if(!RHS) return nullptr;

        int NextPrec = GetTokPrecedence();
        if(NextPrec > ExprPrec){
            RHS = ParseBinOpRHS(ExprPrec + 1, std::move(RHS));
            if(!RHS) return nullptr;
        }

        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
    }

    return LHS;
}

// ParseExpression — the main expression entry point.
//
// It should:
//   1. Call ParsePrimary() to get the left-hand side
//   2. Pass it to ParseBinOpRHS(0, ...) to pick up any binary operators
//
// You do not need to handle operator precedence here — ParseBinOpRHS does that.
// This function should be very short (2-3 lines).
static std::unique_ptr<ExprAST> ParseExpression() {
    
    return ParseBinOpRHS(0, ParsePrimary());
}

// ParsePrototype — parse a function signature.
//
// Called when: CurTok == tok_identifier (the function name), after seeing `Do`
// It should:
//   1. Save the function name, expect and consume '('
//   2. Read comma-separated identifier names until ')'
//   3. Consume ')' and return a PrototypeAST
//
// Input:  fibonacci(n)
// Output: Prototype(fibonacci(n))
//
// Input:  add(a, b)
// Output: Prototype(add(a, b))
//
// Input:  commence()
// Output: Prototype(commence())
static std::unique_ptr<PrototypeAST> ParsePrototype() {
    
    if(CurTok == tok_identifier){
        std::string func_name = IdentifierStr;
        getNextToken();
        if(CurTok != '(') return LogErrorP("Expected '(' in ParsePrototype");

        std::vector<std::string> args;
        while(CurTok != ')'){
            if(CurTok == tok_identifier){
                args.push_back(IdentifierStr);
            }
            getNextToken();
        }

        return std::make_unique<PrototypeAST>(func_name, args);
    }
    return LogErrorP("Identifier Expected ParsePrototype");
}

// ParseDefinition — parse a full `Do functionName(args) { body }` definition.
static std::unique_ptr<FunctionAST> ParseDefinition() {
    
    if(CurTok == tok_do){
        getNextToken();
        auto proto = ParsePrototype();
        getNextToken();

        //TODO: if(CurTok != '{') return std::cout<<"Error in Parse Defition";
        getNextToken();
        auto body = ParseExpression();

        //TODO: if(CurTok != '}') return std::cout<<"Error in Parse Defition";
        return std::make_unique<FunctionAST>(std::move(proto), std::move(body));
    }
    return nullptr;
}

static void resetParser(const std::string &src) {
    g_inputBuf = src;
    g_inputPos = 0;
    g_lastChar = ' ';
    CurTok = 0;
}


int main() {

    return 0;
}