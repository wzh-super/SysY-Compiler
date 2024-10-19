#pragma once

#include <memory>
#include <string>
#include<iostream>
using namespace std;

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const =0;
    virtual void GenerateIR(string& s) const=0;
};

class CompUnitAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override{
        std::cout<<"CompUnitAST { ";
        func_def->Dump();
        std::cout<<" }";
    }

    void GenerateIR(string& s) const override{
        func_def->GenerateIR(s);
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

    void GenerateIR(string& s) const override{
        s+="fun @"+ident+"(): ";
        functype->GenerateIR(s);
        s+=" {\n";
        block->GenerateIR(s);
        s+="}";
    }
};

class FuncTypeAST:public BaseAST{
public:
    std::string type;

    void Dump() const override{
        std::cout<<"FuncTypeAST { "<<type<<" }";
    }

    void GenerateIR(string& s) const override{
        if (type=="int")
            s+="i32";
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

    void GenerateIR(string& s) const override{
        s+="%entry:\n";
        stmt->GenerateIR(s);
    }
};

class StmtAST:public BaseAST{
public:
    std::string ret;
    int number;

    void Dump() const override{
        std::cout<<"StmtAST { "<<number<<" }";
    }

    void GenerateIR(string& s) const override{
        s+="  ret "+std::to_string(number)+"\n";
    }
};

class NumberAST:public BaseAST{
public:
    int int_const;
};