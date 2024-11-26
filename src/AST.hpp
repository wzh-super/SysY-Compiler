#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>

#define NOTFIND 0xffffffff
#define COMPUTEERROR 0xfffffffe

using namespace std;

static int val_num=0;

class SymbolTable{
public:
    SymbolTable* parent=nullptr;
    map<std::string,int> table;
    std::vector<SymbolTable*> children;
    
    ~SymbolTable(){
        for (auto& child:children){
            delete child;
        }
    }

    void Insert(const std::string& name,int value){
        table[name]=value;
    }

    bool isExist(const std::string& name){
        if (table.find(name)!=table.end())
            return true;
        else if(parent!=nullptr)
            return parent->isExist(name);
        else
            return false;
    }

    int query(const std::string& name){
        if (table.find(name)!=table.end())
            return table[name];
        else if (parent!=nullptr)
            return parent->query(name);
        else
            return NOTFIND;
    }

    SymbolTable* AddChild(){
        SymbolTable* child=new SymbolTable();
        child->parent=this;
        children.push_back(child);
        return child;
    }

    void RemoveChild(SymbolTable* child){
        for (auto it=children.begin();it!=children.end();++it){
            if (*it==child){
                children.erase(it);
                delete child;
                return;
            }
        }
    }
};

static SymbolTable global_table;

class BaseAST {
public:
    mutable SymbolTable* symbol_table;
    virtual ~BaseAST() {};
    virtual void Dump() const =0;
    virtual std::string GenerateIR(string& s) const=0;
    virtual int compute_exp() const { return 0; }
    virtual void set_symbol_table(SymbolTable* table){}
};

class CompUnitAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override{
        std::cout<<"CompUnitAST { ";
        func_def->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=&global_table;
        func_def->set_symbol_table(symbol_table->AddChild());
    }

    std::string GenerateIR(string& s) const override{
        // symbol_table=&global_table;
        // func_def->symbol_table=symbol_table->AddChild();
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

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        block->set_symbol_table(symbol_table);
    }

    std::string GenerateIR(string& s) const override{
        s+="fun @"+ident+"(): ";
        functype->GenerateIR(s);
        s+=" {\n";
        // block->symbol_table=symbol_table;
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
    std::vector<std::unique_ptr<BaseAST>> blockitem;

    void Dump() const override{
        std::cout<<"BlockAST { ";
        for (const auto& b_item:blockitem){
            b_item->Dump();
            std::cout<<", ";
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for (const auto& b_item:blockitem){
            b_item->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s) const override{
        //TODO
        s+="%entry:\n";
        for (const auto& b_item:blockitem){
            // b_item->symbol_table=symbol_table;
            b_item->GenerateIR(s);
        }
        return "";
    }
};

class BlockItemAST:public BaseAST{
public:
    enum class Kind{Decl,Stmt};
    Kind kind;
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override{
        std::cout<<"BlockItemAST { ";
        switch (kind){
            case Kind::Decl:
                decl->Dump();
                break;
            case Kind::Stmt:
                stmt->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Decl:
                decl->set_symbol_table(symbol_table);
                break;
            case Kind::Stmt:
                stmt->set_symbol_table(symbol_table);
                break;
        }
    }

    string GenerateIR(string& s) const override{
        switch (kind){
            case Kind::Decl:
                //TODO
                // decl->symbol_table=symbol_table;
                decl->GenerateIR(s);
                return "";
            case Kind::Stmt:
                // stmt->symbol_table=symbol_table;
                stmt->GenerateIR(s);
                return "";
        }
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

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        exp->set_symbol_table(symbol_table);
    }

    string GenerateIR(string& s) const override{
        // exp->symbol_table=symbol_table;
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

    int compute_exp() const override{
        return int_const;
    }

    string GenerateIR(string& s) const override{
        
        return to_string(int_const);
    }
};

class ExpAST:public BaseAST{
public:
    // enum class Kind{Unary,Add,Or};
    // Kind kind;
    // std::unique_ptr<BaseAST> unaryexp;
    // std::unique_ptr<BaseAST> addexp;
    std::unique_ptr<BaseAST> orexp;

    void Dump() const override{
        std::cout<<"ExpAST { ";
        orexp->Dump();
        // switch (kind){
        //     case Kind::Unary:
        //         unaryexp->Dump();
        //         break;
        //     case Kind::Add:
        //         addexp->Dump();
        //         break;
        //     case Kind::Or:
        //         orexp->Dump();
        //         break;
        // }
        std::cout<<" }";
    }

    int compute_exp() const override{
        return orexp->compute_exp();
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        orexp->set_symbol_table(symbol_table);
    }

    //生成IR时，返回目前变量的名字
    string GenerateIR(string& s) const override{
        string value;
        // switch (kind){
        //     case Kind::Unary:
        //         unaryexp->symbol_table=symbol_table;
        //         value=unaryexp->GenerateIR(s);
        //         break;
        //     case Kind::Add:
        //         addexp->symbol_table=symbol_table;
        //         value=addexp->GenerateIR(s);
        //         break;
        //     case Kind::Or:
        //         orexp->symbol_table=symbol_table;
        //         value=orexp->GenerateIR(s);
        //         break;
        // }
        // orexp->symbol_table=symbol_table;
        value=orexp->GenerateIR(s);
        return value;
    }
};

class PrimaryExpAST:public BaseAST{
public:
    enum class Kind{Number,Exp,LVal};
    Kind kind;
    std::unique_ptr<BaseAST> number;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;

    void Dump() const override{
        std::cout<<"PrimaryExpAST { ";
        switch (kind){
            case Kind::Number:
                number->Dump();
                break;
            case Kind::Exp:
                exp->Dump();
                break;
            case Kind::LVal:
                lval->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Exp:
                exp->set_symbol_table(symbol_table);
                break;
            case Kind::LVal:
                lval->set_symbol_table(symbol_table);
                break;
            default:
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Number:
                return number->compute_exp();
            case Kind::Exp:
                return exp->compute_exp();
            case Kind::LVal:
                return lval->compute_exp();
        }
    }

    string GenerateIR(string& s) const override{
        string current_val;
        string next_val;
        switch (kind){
            case Kind::Number:
                current_val=number->GenerateIR(s);
                break;
            case Kind::Exp:
                // exp->symbol_table=symbol_table;
                current_val=exp->GenerateIR(s);
                break;
            case Kind::LVal:
                // lval->symbol_table=symbol_table;
                current_val=lval->GenerateIR(s);
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

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Primary:
                primaryexp->set_symbol_table(symbol_table);
                break;
            case Kind::Unary:
                unaryexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Primary:
                return primaryexp->compute_exp();
            case Kind::Unary:
                if (unaryop=="-")
                    return -unaryexp->compute_exp();
                else if (unaryop=="!")
                    return !unaryexp->compute_exp();
                else if (unaryop=="+")
                    return unaryexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        string current_val;
        string next_val;
        switch (kind){
            case Kind::Primary:
                // primaryexp->symbol_table=symbol_table;
                current_val=primaryexp->GenerateIR(s);
                next_val=current_val;
                break;
            case Kind::Unary:
                // unaryexp->symbol_table=symbol_table;
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

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Unary:
                unaryexp->set_symbol_table(symbol_table);
                break;
            case Kind::Mult:
                mulexp->set_symbol_table(symbol_table);
                unaryexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Unary:
                return unaryexp->compute_exp();
            case Kind::Mult:
                if (op=="*")
                    return mulexp->compute_exp()*unaryexp->compute_exp();
                else if (op=="/")
                    return mulexp->compute_exp()/unaryexp->compute_exp();
                else if (op=="%")
                    return mulexp->compute_exp()%unaryexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Unary:
                // unaryexp->symbol_table=symbol_table;
                current_val_2=unaryexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Mult:
                // mulexp->symbol_table=symbol_table;
                // unaryexp->symbol_table=symbol_table;
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

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Mul:
                mulexp->set_symbol_table(symbol_table);
                break;
            case Kind::Add:
                addexp->set_symbol_table(symbol_table);
                mulexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Mul:
                return mulexp->compute_exp();
            case Kind::Add:
                if (op=="+")
                    return addexp->compute_exp()+mulexp->compute_exp();
                else if (op=="-")
                    return addexp->compute_exp()-mulexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Mul:
                // mulexp->symbol_table=symbol_table;
                current_val_2=mulexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Add:
                // addexp->symbol_table=symbol_table;
                // mulexp->symbol_table=symbol_table;
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

class RelExpAST:public BaseAST{
public:
    enum class Kind{Add,Rel};
    Kind kind;
    std::unique_ptr<BaseAST> addexp;
    std::string op;
    std::unique_ptr<BaseAST> relexp;

    void Dump() const override{
        std::cout<<"RelExpAST { ";
        switch (kind){
            case Kind::Add:
                addexp->Dump();
                break;
            case Kind::Rel:
                relexp->Dump();
                std::cout<<op;
                addexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Add:
                addexp->set_symbol_table(symbol_table);
                break;
            case Kind::Rel:
                relexp->set_symbol_table(symbol_table);
                addexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Add:
                return addexp->compute_exp();
            case Kind::Rel:
                if (op=="<")
                    return relexp->compute_exp()<addexp->compute_exp();
                else if (op==">")
                    return relexp->compute_exp()>addexp->compute_exp();
                else if (op=="<=")
                    return relexp->compute_exp()<=addexp->compute_exp();
                else if (op==">=")
                    return relexp->compute_exp()>=addexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Add:
                // addexp->symbol_table=symbol_table;
                current_val_2=addexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Rel:
                // relexp->symbol_table=symbol_table;
                // addexp->symbol_table=symbol_table;
                current_val_1=relexp->GenerateIR(s);
                current_val_2=addexp->GenerateIR(s);
                next_val="%"+to_string(val_num++);
                if (op=="<"){
                    s+="  "+next_val+" = lt "+current_val_1+", "+current_val_2+"\n";
                }
                else if (op==">"){
                    s+="  "+next_val+" = gt "+current_val_1+", "+current_val_2+"\n";
                }
                else if (op=="<="){
                    s+="  "+next_val+" = le "+current_val_1+", "+current_val_2+"\n";
                }
                else if (op==">="){
                    s+="  "+next_val+" = gt "+current_val_1+", "+current_val_2+"\n";
                }
                break;
        }
        return next_val;
    }
};

class EqExpAST:public BaseAST{
public:
    enum class Kind{Rel,Eq};
    Kind kind;
    std::unique_ptr<BaseAST> relexp;
    std::string op;
    std::unique_ptr<BaseAST> eqexp;

    void Dump() const override{
        std::cout<<"EqExpAST { ";
        switch (kind){
            case Kind::Rel:
                relexp->Dump();
                break;
            case Kind::Eq:
                eqexp->Dump();
                std::cout<<op;
                relexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Rel:
                relexp->set_symbol_table(symbol_table);
                break;
            case Kind::Eq:
                eqexp->set_symbol_table(symbol_table);
                relexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Rel:
                return relexp->compute_exp();
            case Kind::Eq:
                if (op=="==")
                    return eqexp->compute_exp()==relexp->compute_exp();
                else if (op=="!=")
                    return eqexp->compute_exp()!=relexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s)const override{
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Rel:
                // relexp->symbol_table=symbol_table;
                current_val_2=relexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Eq:
                // eqexp->symbol_table=symbol_table;
                // relexp->symbol_table=symbol_table;
                current_val_1=eqexp->GenerateIR(s);
                current_val_2=relexp->GenerateIR(s);
                next_val="%"+to_string(val_num++);
                if (op=="=="){
                    s+="  "+next_val+" = eq "+current_val_1+", "+current_val_2+"\n";
                }
                else if (op=="!="){
                    s+="  "+next_val+" = ne "+current_val_1+", "+current_val_2+"\n";
                }
                break;
        }
        return next_val;
    }
};

class LAndExpAST:public BaseAST{
public:
    enum class Kind{Eq,And};
    Kind kind;
    std::unique_ptr<BaseAST> eqexp;
    std::unique_ptr<BaseAST> landexp;

    void Dump() const override{
        std::cout<<"LAndExpAST { ";
        switch (kind){
            case Kind::Eq:
                eqexp->Dump();
                break;
            case Kind::And:
                landexp->Dump();
                std::cout<<"&&";
                eqexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Eq:
                eqexp->set_symbol_table(symbol_table);
                break;
            case Kind::And:
                landexp->set_symbol_table(symbol_table);
                eqexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::Eq:
                return eqexp->compute_exp();
            case Kind::And:
                return landexp->compute_exp()&&eqexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Eq:
                // eqexp->symbol_table=symbol_table;
                current_val_2=eqexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::And:
                // landexp->symbol_table=symbol_table;
                // eqexp->symbol_table=symbol_table;
                current_val_1=landexp->GenerateIR(s);
                current_val_2=eqexp->GenerateIR(s);
                string tmp_val_1="%"+to_string(val_num++);
                s+="  "+tmp_val_1+" = ne "+current_val_1+", 0\n";
                string tmp_val_2="%"+to_string(val_num++);
                s+="  "+tmp_val_2+" = ne "+current_val_2+", 0\n";
                next_val="%"+to_string(val_num++);
                s+="  "+next_val+" = and "+tmp_val_1+", "+tmp_val_2+"\n";
                break;
        }
        return next_val;
    }
};

class LOrExpAST:public BaseAST{
public:
    enum class Kind{And,Or};
    Kind kind;
    std::unique_ptr<BaseAST> landexp;
    std::unique_ptr<BaseAST> lorexp;

    void Dump() const override{
        std::cout<<"LOrExpAST { ";
        switch (kind){
            case Kind::And:
                landexp->Dump();
                break;
            case Kind::Or:
                lorexp->Dump();
                std::cout<<"||";
                landexp->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::And:
                landexp->set_symbol_table(symbol_table);
                break;
            case Kind::Or:
                lorexp->set_symbol_table(symbol_table);
                landexp->set_symbol_table(symbol_table);
                break;
        }
    }

    int compute_exp() const override{
        switch(kind){
            case Kind::And:
                return landexp->compute_exp();
            case Kind::Or:
                return lorexp->compute_exp()||landexp->compute_exp();
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::And:
                // landexp->symbol_table=symbol_table;
                current_val_2=landexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Or:
                // lorexp->symbol_table=symbol_table;
                // landexp->symbol_table=symbol_table;
                current_val_1=lorexp->GenerateIR(s);
                current_val_2=landexp->GenerateIR(s);
                string tmp_val_1="%"+to_string(val_num++);
                s+="  "+tmp_val_1+" = or "+current_val_1+", "+current_val_2+"\n";
                next_val="%"+to_string(val_num++);
                s+="  "+next_val+" = ne "+tmp_val_1+", 0\n";
                break;
        }
        return next_val;
    }
};

class DeclAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> constdecl;

    void Dump() const override{
        std::cout<<"DeclAST { ";
        constdecl->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        constdecl->set_symbol_table(symbol_table);
    }

    string GenerateIR(string& s) const override{
        //TODO
        // constdecl->symbol_table=symbol_table;
        constdecl->GenerateIR(s);
        return "";
    }
};

class ConstDeclAST:public BaseAST{
public:
    std::string btype;
    std::vector<std::unique_ptr<BaseAST>> constdef;

    void Dump() const override{
        std::cout<<"ConstDeclAST { "<<btype;
        for (const auto& c_def:constdef){
            c_def->Dump();
            std::cout<<", ";
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for (const auto& c_def:constdef){
            c_def->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s) const override{
        //TODO
        for (const auto& c_def:constdef){
            // c_def->symbol_table=symbol_table;
            c_def->GenerateIR(s);
        }
        return "";
    }
};

class ConstDefAST:public BaseAST{
public:
    std::string ident;
    std::unique_ptr<BaseAST> constinitval;

    void Dump() const override{
        std::cout<<"ConstDefAST { "<<ident<<", ";
        constinitval->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        constinitval->set_symbol_table(symbol_table);
    }

    int compute_exp() const override{
        return constinitval->compute_exp();
    }

    string GenerateIR(string& s) const override{
        //TODO
        //constinitval->symbol_table=symbol_table;
        symbol_table->Insert(ident,constinitval->compute_exp());
        return "";
        // constinitval->GenerateIR(s);
        // return "";
    }
};

class ConstInitValAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> constexp;

    void Dump() const override{
        std::cout<<"ConstInitValAST { ";
        constexp->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        constexp->set_symbol_table(symbol_table);
    }

    int compute_exp() const override{
        return constexp->compute_exp();
    }

    string GenerateIR(string& s) const override{
        //TODO
        // constexp->symbol_table=symbol_table;
        constexp->GenerateIR(s);
        return "";
    }
};

class LValAST:public BaseAST{
public:
    std::string ident;

    void Dump() const override{
        std::cout<<"LValAST { "<<ident<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
    }

    int compute_exp() const override{
        if(symbol_table->isExist(ident)){
            return symbol_table->query(ident);
        }
        else{
            assert(false);
        }
    }

    string GenerateIR(string& s) const override{
        //TODO
        if(symbol_table->isExist(ident)){
            return to_string(symbol_table->query(ident));
        }
        else{
            assert(false);
        }
        return "";
    }
};

class ConstExpAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override{
        std::cout<<"ConstExpAST { ";
        exp->Dump();
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        exp->set_symbol_table(symbol_table);
    }

    int compute_exp() const override{
        return exp->compute_exp();
    }

    string GenerateIR(string& s) const override{
        //TODO
        // exp->symbol_table=symbol_table;
        exp->GenerateIR(s);
        return "";
    }
};