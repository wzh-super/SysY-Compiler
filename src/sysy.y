%code requires {
    #include <memory>
    #include <string>
    #include "AST.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <vector>
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
    vector<std::unique_ptr<BaseAST>>* vec_val;
}

%token INT RETURN CONST
%token <str_val> IDENT RELOP EQOP ANDOP OROP
%token <int_val> INT_CONST

%type <ast_val> FuncDef FuncType Block BlockItem Stmt Exp UnaryExp PrimaryExp Number MulExp AddExp RelExp 
                EqExp LAndExp LOrExp Decl ConstDecl ConstDef ConstInitVal LVal ConstExp VarDecl VarDef Initval

%type <str_val> UnaryOp MulOp AddOp BType

%type <vec_val> ConstDefList BlockItemList VarDefList

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
    : '{' BlockItemList '}'
    {
        auto ast=new BlockAST();
        vector<unique_ptr<BaseAST>>* vec=$2;
        ast->blockitem=move(*vec);
        delete vec;
        $$=ast;
    }
    ;

BlockItemList
    : 
    {
        auto vec=new vector<unique_ptr<BaseAST>>();
        
        $$=vec;
    }
    | BlockItemList BlockItem
    {
        vector<unique_ptr<BaseAST>>* vec=$1;
        vec->push_back(unique_ptr<BaseAST>($2));
        $$=vec;
    }
    ;

BlockItem
    : Decl
    {
        auto ast=new BlockItemAST();
        ast->kind=BlockItemAST::Kind::Decl;
        ast->decl=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | Stmt
    {
        auto ast=new BlockItemAST();
        ast->kind=BlockItemAST::Kind::Stmt;
        ast->stmt=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

Stmt
    : RETURN Exp ';'
    {
        auto ast=new StmtAST();
        ast->kind=StmtAST::Kind::Return;
        ast->ret="ret";
        ast->exp=unique_ptr<BaseAST>($2);
        $$=ast;
    }
    | LVal '=' Exp ';'
    {
        auto ast=new StmtAST();
        ast->kind=StmtAST::Kind::Assign;
        ast->lval=unique_ptr<BaseAST>($1);
        ast->exp=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    | RETURN ';'
    {
        auto ast=new StmtAST();
        ast->kind=StmtAST::Kind::Return;
        ast->ret="ret";
        ast->exp=nullptr;
        $$=ast;
    }
    | Exp ';'
    {
        auto ast=new StmtAST();
        ast->kind=StmtAST::Kind::Exp;
        ast->exp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | ';'
    {
        auto ast=new StmtAST();
        ast->kind=StmtAST::Kind::Exp;
        ast->exp=nullptr;
        $$=ast;
    }
    | Block
    {
        auto ast=new StmtAST();
        ast->kind=StmtAST::Kind::Block;
        ast->block=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

Exp
    : LOrExp
    {
        auto ast=new ExpAST();
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
    | LVal
    {
        auto ast=new PrimaryExpAST();
        ast->kind=PrimaryExpAST::Kind::LVal;
        ast->lval=unique_ptr<BaseAST>($1);
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

Decl
    : ConstDecl
    {
        auto ast=new DeclAST();
        ast->kind=DeclAST::Kind::Const;
        ast->constdecl=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    | VarDecl
    {
        auto ast=new DeclAST();
        ast->kind=DeclAST::Kind::Var;
        ast->vardecl=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

ConstDefList
    : ConstDef
    {
        auto vec=new vector<unique_ptr<BaseAST>>();
        vec->push_back(unique_ptr<BaseAST>($1));
        $$=vec;
    }
    | ConstDefList ',' ConstDef
    {
        vector<unique_ptr<BaseAST>>* vec=$1;
        vec->push_back(unique_ptr<BaseAST>($3));
        $$=vec;
    }
    ;

ConstDecl
    : CONST BType ConstDefList ';'
    {
        auto ast=new ConstDeclAST();
        ast->btype=*unique_ptr<string>($2);
        vector<std::unique_ptr<BaseAST>>* vec=$3;
        ast->constdef=move(*vec);
        delete vec;
        $$=ast;
    }
    ;

BType
    : INT
    {
        std::string* str=new string("int");
        $$=str;
    }
    ;

ConstDef
    : IDENT '=' ConstInitVal
    {
        auto ast=new ConstDefAST();
        ast->ident=*unique_ptr<string>($1);
        ast->constinitval=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    ;

ConstInitVal
    : ConstExp
    {
        auto ast=new ConstInitValAST();
        ast->constexp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

ConstExp
    : Exp
    {
        auto ast=new ConstExpAST();
        ast->exp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

LVal
    : IDENT
    {
        auto ast=new LValAST();
        ast->ident=*unique_ptr<string>($1);
        $$=ast;   
    }
    ;

VarDefList
    : VarDef
    {
        auto vec=new vector<unique_ptr<BaseAST>>();
        vec->push_back(unique_ptr<BaseAST>($1));
        $$=vec;
    }
    | VarDefList ',' VarDef
    {
        vector<unique_ptr<BaseAST>>* vec=$1;
        vec->push_back(unique_ptr<BaseAST>($3));
        $$=vec;
    }
    ;

VarDef
    : IDENT
    {
        auto ast=new VarDefAST();
        ast->kind=VarDefAST::Kind::Ident;
        ast->ident=*unique_ptr<string>($1);
        $$=ast;
    }
    | IDENT '=' Initval
    {
        auto ast=new VarDefAST();
        ast->kind=VarDefAST::Kind::Init;
        ast->ident=*unique_ptr<string>($1);
        ast->initval=unique_ptr<BaseAST>($3);
        $$=ast;
    }
    ;

Initval
    : Exp
    {
        auto ast=new InitValAST();
        ast->exp=unique_ptr<BaseAST>($1);
        $$=ast;
    }
    ;

VarDecl
    : BType VarDefList ';'
    {
        auto ast=new VarDeclAST();
        ast->btype=*unique_ptr<string>($1);
        vector<unique_ptr<BaseAST>>* vec=$2;
        ast->vardef=move(*vec);
        delete vec;
        $$=ast;
    }
    ;

%%

void yyerror(unique_ptr<BaseAST>& ast,const char* s){
    cerr<<"error: "<<s<<endl;
};