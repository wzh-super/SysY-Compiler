%code requires {
    #include <memory>
    #include <string>
    #include "AST.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "AST.hpp"

int yylex();
void yyerror(std::unique_ptr<BaseAST>& ast,const char* s);

using namespace std;

%}

%parse-param {std::unique_ptr<BaseAST>& ast}

%union {
    int int_val;
    std::string* str_val;
    BaseAST* ast_val;
}

%token INT RETURN
%token <str_val> IDENT
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block Stmt Exp UnaryExp PrimaryExp Number

%type <str_val> UnaryOp


%%

CompUnit
    : FuncDef{
        auto comp_unit=make_unique<CompUnitAST>();
        comp_unit->func_def=unique_ptr<BaseAST>($1);
        ast=move(comp_unit);
    }
    ;

FuncDef
    : FuncType IDENT '(' ')' Block
    {
        auto ast=new FuncDefAST();
        ast->functype=unique_ptr<BaseAST>($1);
        ast->ident=*unique_ptr<string>($2);
        ast->block=unique_ptr<BaseAST>($5);
        $$=ast;
    }
    ;

FuncType
    : INT
    {
        auto ast=new FuncTypeAST();
        ast->type="int";
        $$=ast;
    }
    ;

Block
    : '{' Stmt '}'
    {
        auto ast=new BlockAST();
        ast->stmt=unique_ptr<BaseAST>($2);
        $$=ast;
    }
    ;

Stmt
    : RETURN Exp ';'
    {
        auto ast=new StmtAST();
        ast->ret="ret";
        ast->exp=unique_ptr<BaseAST>($2);
        $$=ast;
    }
    ;

Exp
    : UnaryExp
    {
        auto ast=new ExpAST();
        ast->unaryexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

UnaryExp
    : PrimaryExp
    {
        auto ast=new UnaryExpAST();
        ast->kind=UnaryExpAST::Kind::Primary;
        ast->primaryexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | UnaryOp UnaryExp
    {
        auto ast=new UnaryExpAST();
        ast->kind=UnaryExpAST::Kind::Unary;
        ast->unaryop=*$1;
        ast->unaryexp=unique_ptr<BaseAST>($2);
        $$=ast;
    }
    ;

PrimaryExp
    : '(' Exp ')'
    {
        auto ast=new PrimaryExpAST();
        ast->kind=PrimaryExpAST::Kind::Exp;
        ast->exp=unique_ptr<BaseAST>($2);
        $$=ast;
    }
    | Number
    {
        auto ast=new PrimaryExpAST();
        ast->kind=PrimaryExpAST::Kind::Number;
        ast->number=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

UnaryOp
    : '+'
    {
        string s="+";
        $$=&s;
    }
    | '-'
    {
        string s="-";
        $$=&s;
    }
    | '!'
    {
        string s="!";
        $$=&s;
    }
    ;

Number
    : INT_CONST
    {
        auto ast=new NumberAST();
        ast->int_const=$1;
        $$=ast;
    }
    ;

%%

void yyerror(unique_ptr<BaseAST>& ast,const char* s){
    cerr<<"error: "<<s<<endl;
};