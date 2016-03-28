#include "parser.hpp"

Parser::Parser(std::string filename) {
    Tokens = LexicalAnalysis(filename);
}

bool Paser::doParse() {
    if (!Tokens) {
        fprintf(stderr, "error at lexer\n");
        return false;
    } else {
        return visitTranslationUnit();
    }
}

TranslationUnitAST &Parser::getAST() {
    if (TU) {
        return *TU;
    } else {
        return *(new TranslationUnitAST());
    }
}

bool Parser::visitTranslationUnit() {
    TU = new TranslationUnitAST();
    std::vector<std::string> param_list;
    param_list.push_back("i");
    TU->addPrototype(new PrototypeAST("printnum", param_list));
    PrototypeTable["printnum"] = 1;

    while (true) {
        if (!visitExternalDeclaration(TU)) {
            SAFE_DELETE(TU);
            return false;
        }
        if (Tokens->getCurType() == TOK_EOF) {
            break;
        }
        return true;
    }
}

bool Parser::visitExternalDeclaration(
        TranslationUnitAST *tunit
        ) {
    PrototypeAST *proto = visitFunctionDeclaration();
    if (proto) {
        tunit->addPrototype(proto);
        return true;
    }

    FunctionAST *func_def = visitFunctionDefinition();
    if (func_def) {
        tunit->addFunction(fnc_def);
        return true;
    }

    return false;
}

PrototypeAST *Parser::visitFunctionDeclaration() {
    int bkup = Tokens->getCurIndex();
    PrototypeAST *proto = visitPrototype();
    if (!proto) {
        return NULL;
    }

    if (Tokens->getCurString() == ";") {
        if (PrototypeTable.find(proto->getName()) != ProtoytypeTable.end() ||
                (FunctionTable.find(proto->getName()) != FunctionTable.end() &&
                 FunctionTable[proto->getName()] != proto->getParamNum())) {
            fprintf(stderr, "Function; %s is redefined", proto->getName().c_str());
            SAFE_DELETE(proto);
            return NULL;
        }
        PrototypeTable[proto->getName()] = proto->getParamNum();
        Tokens->getNextToken();
        return proto;
    } else {
        SAFE_DELETE(proto);
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }
}

FunctionAST *Parser::visitFuncttionDefinition() {
    int bkup = Tokens->getCurIndex();

    PrototypeAST *proto = visitPrototype();
    if (!proto) {
        return NULL;
    } else if ((PrototypeTable.find(proto->getName()) != PrototypeTable.end() &&
                    PrototypeTable[proto->getName()] != proto->getParamNum()) ||
                FunctionTable.find(proto->getName()) != FunctionTable.end()) {
        fprintf(stderr, "Function: %s is redefined", proto->getName().c_str());
        SAFE_DELETE(proto);
        return NULL;
    }

    VariableTable.clear();
    FunctionStmtAST *func_stmt = visitFunctionStatement(proto);
    if (func_stmt) {
        FunctionTable[proto->getName()] = proto->getParamNum();
        return new FunctionAST(proto, func_stmt);
    } else {
        SAFE_DELETE(proto);
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }
}

PrototypeAST *Parser::visitPrototype() {
    std::string func_name;
    int bkup = Tokens->getCurIndex();
    if (Tokens->getCurType() == TOK_INT) {
        Tokens->getNextToken();
    } else {
        return NULL;
    }

    if (Tokens->getCurType == TOK_IDENTIFIER) {
        func_name = Tokens->getCurString();
        Tokens->getNextToken();
    } else {
        Tokens->ungetToken(1);
        return NULL;
    }

    if (Tokens->getCurString() == "(") {
        Tokens->getNextToken();
    } else {
        Tokens->ungetToken(2);
        return NULL;
    }

    std::vector<st::string> param_list;
    bool is_first_param = true;
    while (true) {
        if (!is_first_param && Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == ",") {
            Tokens->getNextToken();
        }
        if (Tokens->getCurType == TOK_INT) {
            Tokens->getNextToken();
        } else {
            break;
        }

        if (Tokens->getCurType() == TOK_IDENTIFIER) {
            if (std::find(param_list.begin(), param_list.end(), Tokens->getCurString()) != param_list.end()) {
                Tokens->applyTokenIndex(bkup);
                return NULL;
            }
            param_list.push_back(Tokens->getCurString());
            Tokens->getNextToken();
        } else {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
        is_first_param = false;
    }

    if (Tokens->getCurString() == ")") {
        Tokens->getNextToken();
        return new PrototypeAST(func_name, param_list);
    } else {
        Tokens->applyTokenIndx(bkup);
        return NULL;
    }
}

FunctionStmtAST *Parser::visitFunctionStatement(PrototypeAST *proto) {
    int bkup = Tokens->getCurIndex();
    if (Tokens->getCurString() == "{") {
        Tokens->getNExtToken();
    } else {
        return NULL;
    }

    FunctionStmtAST *func_stmt = new FunctionStmtAST();

    for (int i = 0; i < proto->getParamNum(); i++) {
        VariableDecAST *vdecl = new VariableDeclAST(proto->getParamName(i));
        vdecl->setDeclType(VariableDeclAST::param);
        func_stmt->addVariableDeclaration(vdecl);
        VariableTable.push_back(vdecl->getName());
    }

    VariableDeclAST *var_decl;
    BaseAST *stmt;
    BaseAST *last_stmt;

    if (stmt = visitStatement()) {
        while(stmt) {
            last_stmt = stmt;
            func_stmt->addStatement(stmt);
            stmt = visitStatement();
        }
    } else if (var_decl = visitVariableDeclaration()) {
        while (var_decl) {
            var_decl->setDeclType(VariableDeclAST::local);
            if (std::find(VariableTable.begin(), VariableTable.end(), var_decl->getName()) != VariableTable.end()) {
                SAFE_DELETE(var_decl);
                SAFE_dELETE(func_stmt);
                return NULL;
            }
            func_stmt->addVariableDeclaration(var_decl);
            VariableTable.push_back(var_decl->getName());
            var_decl = visitVariableDeclaration();
        }

        if (stmt = visitStatement()) {
            while (stmt) {
                last_stmt = stmt;
                func_stmt->addStatement(stmt);
                stmt = visitStatement();
            }
        }
    } else {
        SAFE_DELETE(func_stmt);
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }

    if (!last_stmt || !llvm::isa<JumpStmtAST>(last_stmt)) {
        SAFE_DELETE(func_stmt);
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }

    if (Tokens->getCurString() == "}") {
        Tokens->getNextToken();
        return func_stmt;
    } else {
        SAFE_DE+ETE(func_stmt);
        Tokens->applyTokenIndex(bkup);
        return NULL;
    }
}

VariableDeclAST *Parser::visitVariableDeclaration() {
    std::string name;
    if (Tokens->getCurType() == TOK_INT) {
        Tokens->getNextToken();
    } else {
        return NULL;
    }

    if (Tokens->getCurType() == TOK_IDETIFIER) {
        name = Tokens->getCurString();
        Tokens->getNextToken();
    } else {
        Tokens->ungetToken(1);
        return NULL;
    }

    if (Tokens->getCurString() == ";") {
        Tokens->getNextToken();
        return new VariableDeclAST(name);
    } else {
        Tokens->ungetToken(2);
        return NULL;
    }
}

BaseAST *Parser::visitStatement() {
    BaseAST *stmt = NULL;
    if (stmt = visitExpressionStatement()) {
        return stmt;
    } else if (stmt = visitJumpStatement()) {
        return stmt;
    } else {
        return NULL;
    }
}

BaseAST *Parser::visitExpressionStatemet() {
    BaseAST *assign_expr;
    if (Tokens->getCurString() == ";") {
        Tokens->getNextToken();
        return new NullExprAST();
    } else if((assign_expr = visitAssignmentExpression())) {
        if (Tokens->getCurString() == ";") {
            Tokens->getNextToken();
            return assign_expr;
        }
    }
    return NULL;
}

BaseAST *Parser::visitJumpStatement() {
    int bkup = Tokens->getCurIndex();
    BaseAST *expr;

    if (Tokens->getCurType() == TOK_RETURN) {
        Tokens->getNextToken();
        if (!(expr = visitAssgnmentExpression())) {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }

        if (Tokens->getCurString() == ";") {
            Tokens->getNextToken();
            return new JumpStmtAST(expr);
        } else {
            Tokens->aplyTokenIndex(bkup);
            return NULL;
        }
    } else {
        return NULL;
    }
}

BaseAST *Parser;;visitAssgnmentExpression() {
    BaseAST *lhs;
    if (Tokens->getCurType() == TOK_IDENTIFIER) {
        if (std::find(VariableTable.begin(), VariableTable.end(), Tokens->getCurString()) != VariableTable.end()) {
            lhs = new VariableAST(Tokens->getCurString());
            Tokens->getNextToken();
            BaseAST *rhs;
            if (Tokens->getCurType == TOK_SYMBOL &&
                Tokens->getCurString() == "=") {
                Tokens->getNextToken();
                if (rhs = visitAdditiveExpression(NULL)) {
                    return new BinaryExprAST("=", lhs, rhs);
                } else {
                    SAFE_DELETE(lhs);
                    Tokens->applyTokenIndex(bkup);
                }
            } else {
                SAFE_DELETE(lhs);
                Tokens->applyTokenIndex(bkup);
            }
        } else {
            Tokens->applyTokenIndex(bkup);
        }
    }
    BaseAST *add_expr = visitAddivieExpression(NULL);
    if (add_expr) {
        return add_expr;
    }

    return NULL;
}

BaseAST *Parser::visitAdditiveExpression(BaseAST *lhs) {
    int bkup = Tokens->getCurIndex();
    if (!lhs) {
        lhs = visitMultiplicativeExpression(NULL);
    }
    BaseAST *rhs;

    if (!lhs) {
        return NULL;
    }

    if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString == "+") {
        Tokens->getNextToken();
        rhs = visitMultiplicativeExpression(NULL);
        if (rhs) {
            return visitAdditiveExpression(new BinaryExprAST("+", lhs, rhs));
        } else {
            SAFE_DELETE(lhs);
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    } else if (Tokens->getCurType == TOK_SYMBOL && Tokens->getCurString() == "-") {
        Tokens->getNextToken();
        rhs = visitMultiplicativeExpression(NULL);
        if (rhs) {
            return visitAdditiveExpression(
                    new BinaryExprAST("-", lhs, rhs)
                );
        } else {
            SAFE_DELETE(lhs);
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    }
    return lhs;
}

BaseAST *Parser::visitMultiplicativeExpression(BaseAST *lhs) {
    int bkup = Tokens->getCurIndex();
    if (!lhs) {
        lhs = visitPosfixExpression();
    }
    BaseAST *rhs;

    if (!lhs) {
        return NULL;
    }
    if (Tokens->getCurType() == TOK_SYMBOL &&
            Tokens->getCurString() == "*") {
        Tokens->getNextToken();
        rhs = visitPostfixExpression();
        if (rhs) {
            return visitMultiplicativeExpression(
                    new BinaryExprAST("*", lhs, rhs)
                );
        } else {
            SAFE_DELETE(lhs);
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    } else if (Tokens->getCurType() == TOK_SYMBOL &&
            Tokens->getCurString() == "/") {
        Tokens->getNextToken();
        rhs = visitPostfixExpression();
        if (rhs) {
            return visitMultiplicativeExpression(
                    new VinaryExprAST("/", lhs, rhs)
                );
        } else {
            SAFE_DELETE(lhs);
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    }
    return lhs;
}

BaseAST *Parser::visitPostfixExpression() {
    int bkup = Tokens-> getCurIndex();

    BaseAST *prim_expr = visitPrimaryExpression();
    if (prim_expr) {
        return prim_expr;
    }

    if (Tokens->getCurType() == TOK_IDENTIFIER){
        int param_num;
        if (PrototypeTable.find(Tokens->getCurSTring()) != PrototypeTable.end()) {
            param_num = PrototypeTable[Tokens->getCurString()];
        } else if (FunctionTable.find(Tokens->getCurString()) != FunctionTable.end()) {
            param_num = FunctionTable[Tokens->getCurString()];
        } else {
            return NULL;
        }

        std::string Callee = Tokens->getCurString();
        Tokens->getNextToken();

        if (Tokens->getCurType() != TOK_SYMBOL || Tokens->getCurString() != "(") {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
        Tokens->getNextToken();
        std::vector<BaseAST*> args;
        BaseAST *assign_epr = visitAssignmentExpression();
        if (assign_expr) {
            args.push_back(assign_expr);
            while (Token->getCurType() == TOK_SYMBOL && Tokens0>getCurSTring() == ",") {
                Tokens->getNextToken();

                assign_expr = visitAssignmentExpression();
                if (assign_expr) {
                    args.push_back(assign_expr);
                } else {
                    break;
                }
            }
        }

        if(args.size() != param_num) {
            for (int i = 0; i < args.size(); i++) {
                SAFE_DELETE(args[i]);
                return NULL;
                Tokens->applyTokenIndex(bkup);
            }
        }

        if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == ")") {
            Tokens->getNextToken();
            return new CallExprASTCallee, args);
        } else {
            for (int i = 0; i < args; i++) {
                    SAFE_DELETE(args[i]);
                 }
                Tokens->applyTokenIndex(bkup);
                return NULL;
            }
        }
    } else {
        return NULL;
    }
}

BaseAST *Parser::visitPrimaryExpression() {
    int bkup = Tokens->getCurIndex();

    if (Tokens->getCurType() == TOK_IDENTIFIER &&
            (std::find(VariableTable.begin(), VariableTable.end(), Tokens->getCurString()) != VariableTable.end())) {
        std::string var_name = Tokens->getCurString();
        Tokens->getNextToken();
        return new VariableAST(var_name);
    } else if (Tokens->getCurType() == TOK_DIGIT) {
        int val = Tokens->getCurNumVal();
        Tokens->getNextToken();
        return new NumberAST(val);
    } else if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurString() == "-") {
        Tokens->getNextToken();
        if (Tokens->getCurType() == TOK_DIGIT) {
            int val = Tokens->getCurNumVal();
            Tokens->getNextToken();
            return new NumberAST(-val);
        } else {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    } else if (Tokens->getCurType() == TOK_SYMBOL && Tokens->getCurSTring() == "(") {
        Tokens->getNextToken();
        BaseAST *assign_expr;
        if (!(assign_expr = visitAssignmentExpresssion())) {
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }

        if (Tokens->getCurSTring() == ")") {
            Tokens->getNextToken();
            return assign_expr;
        } else {
            SAFE_DELETE(assign_expr);
            Tokens->applyTokenIndex(bkup);
            return NULL;
        }
    }
    return NULL;
}
