#pragma once

#include <memory>
#include <string>
#include<iostream>
using namespace std;

static int val_num=0;

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const =0;
    virtual std::string GenerateIR(string& s) const=0;
};

class CompUnitAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override{
        std::cout<<"CompUnitAST { ";
        func_def->Dump();
        std::cout<<" }";
    }

    std::string GenerateIR(string& s) const override{
        func_def->GenerateIR(s);
        return "";
    }
};

class FuncDefAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> functype;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override{
        std::cout<<"FuncDefAST { ";
        functype->Dump();
        std::cout<<", "<<ident<<", ";
        block->Dump();
        std::cout<<" }";
    }

    std::string GenerateIR(string& s) const override{
        s+="fun @"+ident+"(): ";
        functype->GenerateIR(s);
        s+=" {\n";
        block->GenerateIR(s);
        s+="}";
        return "";
    }
};

class FuncTypeAST:public BaseAST{
public:
    std::string type;

    void Dump() const override{
        std::cout<<"FuncTypeAST { "<<type<<" }";
    }

    string GenerateIR(string& s) const override{
        if (type=="int")
            s+="i32";
        return "";
    }
};

class BlockAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override{
        std::cout<<"BlockAST { ";
        stmt->Dump();
        std::cout<<" }";
    }

    string GenerateIR(string& s) const override{
        s+="%entry:\n";
        stmt->GenerateIR(s);
        return "";
    }
};

class StmtAST:public BaseAST{
public:
    std::string ret;
    std::unique_ptr<BaseAST> exp;
    int number;

    void Dump() const override{
        std::cout<<"StmtAST { "<<number<<" }";
    }

    string GenerateIR(string& s) const override{
        
        string value=exp->GenerateIR(s);
        s+="  ret ";
        s+=value;
        s+='\n';
        return "";
    }
};

class NumberAST:public BaseAST{
public:
    int int_const;

    void Dump() const override{
        std::cout<<int_const;
    }

    string GenerateIR(string& s) const override{
        
        return to_string(int_const);
    }
};

class ExpAST:public BaseAST{
public:
    enum class Kind{Unary,Add};
    Kind kind;
    std::unique_ptr<BaseAST> unaryexp;
    std::unique_ptr<BaseAST> addexp;

    void Dump() const override{
        std::cout<<"ExpAST { ";
        switch (kind){
            case Kind::Unary:
                unaryexp->Dump();
                break;
            case Kind::Add:
                addexp->Dump();
                break;
        }
        std::cout<<" }";
    }
    //生成IR时，返回目前变量的名字
    string GenerateIR(string& s) const override{
        string value;
        switch (kind){
            case Kind::Unary:
                value=unaryexp->GenerateIR(s);
                break;
            case Kind::Add:
                value=addexp->GenerateIR(s);
                break;
        }
        return value;
    }
};

class PrimaryExpAST:public BaseAST{
public:
    enum class Kind{Number,Exp};
    Kind kind;
    std::unique_ptr<BaseAST> number;
    std::unique_ptr<BaseAST> exp;

    void Dump() const override{
        std::cout<<"PrimaryExpAST { ";
        switch (kind){
            case Kind::Number:
                number->Dump();
                break;
            case Kind::Exp:
                exp->Dump();
                break;
        }
        std::cout<<" }";
    }

    string GenerateIR(string& s) const override{
        string current_val;
        string next_val;
        switch (kind){
            case Kind::Number:
                current_val=number->GenerateIR(s);
                break;
            case Kind::Exp:
                current_val=exp->GenerateIR(s);
                break;
        }
        return current_val;
    }
};

class UnaryExpAST:public BaseAST{
public:
    enum class Kind{Primary,Unary};
    Kind kind;
    std::unique_ptr<BaseAST> primaryexp;
    std::string unaryop;
    std::unique_ptr<BaseAST> unaryexp;

    void Dump() const override{
        std::cout<<"UnaryExpAST { ";
        switch (kind){
            case Kind::Primary:
                primaryexp->Dump();
                break;
            case Kind::Unary:
                std::cout<<unaryop;
                unaryexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    string GenerateIR(string& s) const override{
        string current_val;
        string next_val;
        switch (kind){
            case Kind::Primary:
                current_val=primaryexp->GenerateIR(s);
                next_val=current_val;
                break;
            case Kind::Unary:
                current_val=unaryexp->GenerateIR(s);
                if (unaryop=="-"){
                    next_val="%"+to_string(val_num++);
                    s+="  "+next_val+" = sub 0, "+current_val+'\n';
                }
                else if(unaryop=="!"){
                    next_val="%"+to_string(val_num++);
                    s+="  "+next_val+" = eq "+current_val+", 0\n";
                }    
                else if(unaryop=="+"){
                    next_val=current_val;
                }
                break;
        }
        return next_val;
    }
};

class MulExpAST:public BaseAST{
public:
    enum class Kind{Unary,Mult};
    Kind kind;
    std::unique_ptr<BaseAST> unaryexp;
    std::string op;
    std::unique_ptr<BaseAST> mulexp;

    void Dump() const override{
        std::cout<<"MulExpAST { ";
        switch (kind){
            case Kind::Unary:
                unaryexp->Dump();
                break;
            case Kind::Mult:
                mulexp->Dump();
                std::cout<<op;
                unaryexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    string GenerateIR(string& s) const override{
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Unary:
                current_val_2=unaryexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Mult:
                current_val_1=mulexp->GenerateIR(s);
                current_val_2=unaryexp->GenerateIR(s);
                next_val="%"+to_string(val_num++);
                switch (op[0]){
                    case '*':
                        s+="  "+next_val+" = mul "+current_val_1+", "+current_val_2+"\n";
                        break;
                    case '/':
                        s+="  "+next_val+" = div "+current_val_1+", "+current_val_2+"\n";
                        break;
                    case '%':
                        s+="  "+next_val+" = mod "+current_val_1+", "+current_val_2+"\n";
                        break;
                }
        }
        return next_val;
    }
};

class AddExpAST:public BaseAST{
public:
    enum class Kind{Mul,Add};
    Kind kind;
    std::unique_ptr<BaseAST> mulexp;
    std::string op;
    std::unique_ptr<BaseAST> addexp;

    void Dump() const override{
        std::cout<<"AddExpAST { ";
        switch (kind){
            case Kind::Mul:
                mulexp->Dump();
                break;
            case Kind::Add:
                addexp->Dump();
                std::cout<<op;
                mulexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    string GenerateIR(string& s) const override{
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Mul:
                current_val_2=mulexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Add:
                current_val_1=addexp->GenerateIR(s);
                current_val_2=mulexp->GenerateIR(s);
                next_val="%"+to_string(val_num++);
                switch (op[0]){
                    case '+':
                        s+="  "+next_val+" = add "+current_val_1+", "+current_val_2+"\n";
                        break;
                    case '-':
                        s+="  "+next_val+" = sub "+current_val_1+", "+current_val_2+"\n";
                        break;
                }
        }
        return next_val;
    }
};