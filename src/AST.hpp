#pragma once

#include <memory>
#include <string>
#include<iostream>

class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const =0;
    virtual void GenerateIR() const=0;
};

class CompUnitAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override{
        std::cout<<"CompUnitAST { ";
        func_def->Dump();
        std::cout<<" }";
    }

    void GenerateIR() const override{
        func_def->GenerateIR();
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

    void GenerateIR() const override{
        std::cout<<"fun @"<<ident<<"(): ";
        functype->GenerateIR();
        std::cout<<" {"<< std::endl;
        block->GenerateIR();
        std::cout<<"}";
    }
};

class FuncTypeAST:public BaseAST{
public:
    std::string type;

    void Dump() const override{
        std::cout<<"FuncTypeAST { "<<type<<" }";
    }

    void GenerateIR() const override{
        if (type=="int")
            std::cout<<"i32";
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

    void GenerateIR() const override{
        std::cout<<"%entry:"<<std::endl;
        stmt->GenerateIR();
    }
};

class StmtAST:public BaseAST{
public:
    std::string ret;
    int number;

    void Dump() const override{
        std::cout<<"StmtAST { "<<number<<" }";
    }

    void GenerateIR() const override{
        std::cout<<"  ret "<<number<<std::endl;
    }
};

class NumberAST:public BaseAST{
public:
    int int_const;
};