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
%token <str_val> IDENT RELOP EQOP ANDOP OROP
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block Stmt Exp UnaryExp PrimaryExp Number MulExp AddExp RelExp 
                EqExp LAndExp LOrExp

%type <str_val> UnaryOp MulOp AddOp 


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
        ast->kind=ExpAST::Kind::Unary;
        ast->unaryexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | AddExp
    {
        auto ast=new ExpAST();
        ast->kind=ExpAST::Kind::Add;
        ast->addexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | LOrExp
    {
        auto ast=new ExpAST();
        ast->kind=ExpAST::Kind::Or;
        ast->orexp=unique_ptr<BaseAST>($1);
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
        ast->unaryop=*unique_ptr<string>($1);
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
        string* str=new string("+");
        $$=str;
    }
    | '-'
    {
        string* str=new string("-");
        $$=str;
    }
    | '!'
    {
        string* str=new string("!");
        $$=str;
    }
    ;

MulOp
    : '*'
    {
        string* str=new string("*");
        $$=str;
    }
    | '/'
    {
        string* str=new string("/");
        $$=str;
    }
    | '%'
    {
        string* str=new string("%");
        $$=str;
    }
    ;

AddOp
    : '+'
    {
        string* str=new string("+");
        $$=str;
    }
    | '-'
    {
        string* str=new string("-");
        $$=str;
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

MulExp
    : UnaryExp
    {
        auto ast=new MulExpAST();
        ast->kind=MulExpAST::Kind::Unary;
        ast->unaryexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | MulExp MulOp UnaryExp
    {
        auto ast=new MulExpAST();
        ast->kind=MulExpAST::Kind::Mult;
        ast->mulexp=unique_ptr<BaseAST>($1);
        ast->op=*unique_ptr<string>($2);
        ast->unaryexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
AddExp
    :MulExp
    {
        auto ast=new AddExpAST();
        ast->kind=AddExpAST::Kind::Mul;
        ast->mulexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | AddExp AddOp MulExp
    {
        auto ast=new AddExpAST();
        ast->kind=AddExpAST::Kind::Add;
        ast->addexp=unique_ptr<BaseAST>($1);
        ast->op=*unique_ptr<string>($2);
        ast->mulexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }

RelExp
    :AddExp
    {
        auto ast=new RelExpAST();
        ast->kind=RelExpAST::Kind::Add;
        ast->addexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | RelExp RELOP AddExp
    {
        auto ast=new RelExpAST();
        ast->kind=RelExpAST::Kind::Rel;
        ast->relexp=unique_ptr<BaseAST>($1);
        ast->op=*unique_ptr<string>($2);
        ast->addexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    ;

EqExp
    : RelExp
    {
        auto ast=new EqExpAST();
        ast->kind=EqExpAST::Kind::Rel;
        ast->relexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | EqExp EQOP RelExp
    {
        auto ast=new EqExpAST();
        ast->kind=EqExpAST::Kind::Eq;
        ast->eqexp=unique_ptr<BaseAST>($1);
        ast->op=*unique_ptr<string>($2);
        ast->relexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    ;

LAndExp
    : EqExp
    {
        auto ast=new LAndExpAST();
        ast->kind=LAndExpAST::Kind::Eq;
        ast->eqexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | LAndExp ANDOP EqExp
    {
        auto ast=new LAndExpAST();
        ast->kind=LAndExpAST::Kind::And;
        ast->landexp=unique_ptr<BaseAST>($1);
        ast->eqexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    ;

LOrExp
    : LAndExp
    {
        auto ast=new LOrExpAST();
        ast->kind=LOrExpAST::Kind::And;
        ast->landexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | LOrExp OROP LAndExp
    {
        auto ast=new LOrExpAST();
        ast->kind=LOrExpAST::Kind::Or;
        ast->lorexp=unique_ptr<BaseAST>($1);
        ast->landexp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    ;
%%

void yyerror(unique_ptr<BaseAST>& ast,const char* s){
    cerr<<"error: "<<s<<endl;
};