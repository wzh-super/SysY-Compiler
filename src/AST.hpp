#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>
#include <variant>
#include <stack>

using namespace std;

static int val_num=0;
static int ifelse_num=0;
static bool is_return=false;
static int entry_num=1;
static int while_num=0;
static int break_num=0;
static int continue_num=0;
static stack<int> current_while;

class SymbolTable{
public:
    SymbolTable* parent=nullptr;
    map<std::string,std::variant<int,std::string>> table;
    map<std::string,int> var_table; //存变量值
    std::vector<SymbolTable*> children;
    
    ~SymbolTable(){
        for (auto& child:children){
            delete child;
        }
    }

    void Insert(const std::string& name,variant<int,string> value){
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

    variant<int,string> query(const std::string& name){
        if (table.find(name)!=table.end())
            return table[name];
        else if (parent!=nullptr)
            return parent->query(name);
        else
            assert(false);
    }

    int get_val(const std::string& name){
        if (var_table.find(name)!=var_table.end())
            return var_table[name];
        else if (parent!=nullptr)
            return parent->get_val(name);
        else
            assert(false);
    }

    void set_val(const std::string& name,int value){
        if(table.find(name)!=table.end()){
            var_table[name]=value;
        }
        else if(parent!=nullptr){
            parent->set_val(name,value);
        }
        else{
            assert(false);
        }
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
static map<string,int> var_count;

class BaseAST {
public:
    mutable SymbolTable* symbol_table;
    virtual ~BaseAST() {};
    virtual void Dump() const =0;
    virtual std::string GenerateIR(string& s) const=0;
    virtual int compute_exp() const { return 0; }
    virtual void set_symbol_table(SymbolTable* table){}
    virtual string get_ident() const { return ""; }
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
        symbol_table=table;
        func_def->set_symbol_table(symbol_table->AddChild());
    }

    std::string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
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
        block->set_symbol_table(symbol_table->AddChild());
    }

    std::string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        s+="fun @"+ident+"(): ";
        functype->GenerateIR(s);
        s+=" {\n";
        s+="%entry:\n";
        block->GenerateIR(s);
        if(!is_return){
            s+="  ret 0\n";
        }
        s+="}";
        symbol_table->RemoveChild(block->symbol_table);
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
        // if (is_return)
        //     return "";
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
        // if (is_return)
        //     return "";

        for (const auto& b_item:blockitem){
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
        if(is_return){
            return "";
        }
        switch (kind){
            case Kind::Decl:
                decl->GenerateIR(s);
                return "";
            case Kind::Stmt:
                stmt->GenerateIR(s);
                return "";
        }
        return "";
    }
};

class StmtAST:public BaseAST{
public:
    enum class Kind{Matched,Open};
    Kind kind;
    std::unique_ptr<BaseAST> matched_stmt;
    std::unique_ptr<BaseAST> open_stmt;

    void Dump() const override{
        std::cout<<"StmtAST { ";
        switch (kind){
            case Kind::Matched:
                matched_stmt->Dump();
                break;
            case Kind::Open:
                open_stmt->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Matched:
                matched_stmt->set_symbol_table(symbol_table);
                break;
            case Kind::Open:
                open_stmt->set_symbol_table(symbol_table);
                break;
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        switch (kind){
            case Kind::Matched:
                matched_stmt->GenerateIR(s);
                break;
            case Kind::Open:
                open_stmt->GenerateIR(s);
                break;
        }
        return "";
    }
};

class OpenStmtAST:public BaseAST{
public:
    enum class Kind{If,IfElse,While};
    Kind kind;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> if_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    std::unique_ptr<BaseAST> while_stmt;

    void Dump() const override{
        std::cout<<"OpenStmtAST { ";
        switch (kind){
            case Kind::If:
                std::cout<<"if ";
                exp->Dump();
                std::cout<<" ";
                if_stmt->Dump();
                std::cout<<" }";
                break;
            case Kind::IfElse:
                std::cout<<"if ";
                exp->Dump();
                std::cout<<" ";
                if_stmt->Dump();
                std::cout<<"else ";
                else_stmt->Dump();
                std::cout<<" }";
                break;
            case Kind::While:
                std::cout<<"while ";
                exp->Dump();
                std::cout<<" ";
                while_stmt->Dump();
                std::cout<<" }";
                break;
        }
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        if(exp!=nullptr)
            exp->set_symbol_table(symbol_table);
        if(kind==Kind::If||kind==Kind::IfElse)
            if_stmt->set_symbol_table(symbol_table);
        if(kind==Kind::IfElse)
            else_stmt->set_symbol_table(symbol_table);
        if(kind==Kind::While){
            while_stmt->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s)const override{
        if(kind==Kind::IfElse){
            string value;
            string then_label;
            string else_label;
            string end_label;
            bool stmt1_return=false;
            bool stmt2_return=false;
            value=exp->GenerateIR(s);
            then_label="%then_"+to_string(ifelse_num);
            else_label="%else_"+to_string(ifelse_num);
            end_label="%end_"+to_string(ifelse_num);
            ifelse_num++;
            s+="  br "+value+", "+then_label+", "+else_label+'\n';
            s+=then_label+":\n";
            if_stmt->GenerateIR(s);
            if(!is_return){
                s+="  jump "+end_label+'\n';
            }
            else{
                is_return=false;
                stmt1_return=true;
            }
            s+=else_label+":\n";
            else_stmt->GenerateIR(s);
            if(!is_return){
                s+="  jump "+end_label+'\n';
            }
            else{
                is_return=false;
                stmt2_return=true;
            }
            if(stmt1_return&&stmt2_return){
                is_return=true;
            }
            if(!stmt1_return||!stmt2_return)
                s+=end_label+":\n";
        }
        else if(kind==Kind::If){
            string value;
            string then_label;
            string end_label;
            value=exp->GenerateIR(s);
            then_label="%then_"+to_string(ifelse_num);
            end_label="%end_"+to_string(ifelse_num);
            ifelse_num++;
            s+="  br "+value+", "+then_label+", "+end_label+'\n';
            s+=then_label+":\n";
            if_stmt->GenerateIR(s);
            if(!is_return){
                s+="  jump "+end_label+'\n';
            }
            else{
                is_return=false;
            }
            s+=end_label+":\n";
        }
        else if(kind==Kind::While){
            string while_entry;
            string while_body;
            string end_label;
            current_while.push(while_num);
            while_entry="%while_entry_"+to_string(while_num);
            while_body="%while_body_"+to_string(while_num);
            end_label="%while_end_"+to_string(while_num);
            while_num++;
            s+="  jump "+while_entry+'\n';
            s+=while_entry+":\n";
            string value=exp->GenerateIR(s);
            s+="  br "+value+", "+while_body+", "+end_label+'\n';
            s+=while_body+":\n";
            while_stmt->GenerateIR(s);
            if(!is_return){
                s+="  jump "+while_entry+'\n';
            }
            else{
                is_return=false;
            }
            s+=end_label+":\n";
            current_while.pop();
        }
        return "";
    }
};

class MatchedStmtAST:public BaseAST{
public:
    enum class Kind{Return,Assign,Exp,Block,IfElse,While,Break,Continue};
    Kind kind;
    std::string ret;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<BaseAST> if_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    std::unique_ptr<BaseAST> while_stmt;

    void Dump() const override{
        std::cout<<"MatchedStmtAST { ";
        switch (kind){
            case Kind::Return:
                std::cout<<"return ";
                if(exp!=nullptr)
                    exp->Dump();
                break;
            case Kind::Assign:
                lval->Dump();
                std::cout<<"=";
                exp->Dump();
                break;
            case Kind::Exp:
                if(exp!=nullptr)
                    exp->Dump();
                break;
            case Kind::Block:
                block->Dump();
                break;
            case Kind::IfElse:
                std::cout<<"if ";
                if_stmt->Dump();
                std::cout<<"else ";
                else_stmt->Dump();
                break;
            case Kind::While:
                std::cout<<"while ";
                exp->Dump();
                while_stmt->Dump();
                break;
            case Kind::Break:
                std::cout<<"break"<<endl;
                break;
            case Kind::Continue:
                std::cout<<"continue"<<endl;
                break;
        }
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Return:
                if(exp!=nullptr)
                    exp->set_symbol_table(symbol_table);
                break;
            case Kind::Assign:
                lval->set_symbol_table(symbol_table);
                exp->set_symbol_table(symbol_table);
                break;
            case Kind::Block:
                block->set_symbol_table(symbol_table->AddChild());
                break;
            case Kind::Exp:
                if(exp!=nullptr)
                    exp->set_symbol_table(symbol_table);
                break;
            case Kind::IfElse:
                exp->set_symbol_table(symbol_table);
                if_stmt->set_symbol_table(symbol_table);
                else_stmt->set_symbol_table(symbol_table);
                break;
            case Kind::While:
                exp->set_symbol_table(symbol_table);
                while_stmt->set_symbol_table(symbol_table);
                break;
            default:
                break;
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string value;
        string lval_name;
        string then_label;
        string else_label;
        string end_label;
        string while_entry;
        string while_body;
        int find_current_while;
        bool stmt1_return=false;
        bool stmt2_return=false;
        // if(exp!=nullptr)
        //     value=exp->GenerateIR(s);
        switch (kind){
            case Kind::Return:   
                if(exp!=nullptr)
                    value=exp->GenerateIR(s);
                s+="  ret ";
                if(exp!=nullptr)
                    s+=value;
                s+='\n';
                is_return=true;
                return "";
            case Kind::Assign:
                if(exp!=nullptr)
                    value=exp->GenerateIR(s);
                lval_name=get<string>(symbol_table->query(lval->get_ident()));
                s+="  store "+value+", "+lval_name+'\n';
                // symbol_table->var_table[lval->get_ident()]=exp->compute_exp();
                symbol_table->set_val(lval->get_ident(),exp->compute_exp());
                return "";
            case Kind::Block:
                block->GenerateIR(s);
                symbol_table->RemoveChild(block->symbol_table);
                return "";
            case Kind::Exp:
                if(exp!=nullptr)
                    value=exp->GenerateIR(s);
                return value;
            case Kind::IfElse:
                if(exp!=nullptr)
                    value=exp->GenerateIR(s);
            //试图用全局的is_return来判断是否有return语句，但是在ifelse语句中，if和else都有可能有return语句
                then_label="%then_"+to_string(ifelse_num);
                else_label="%else_"+to_string(ifelse_num);
                end_label="%end_"+to_string(ifelse_num);
                ifelse_num++;
                s+="  br "+value+", "+then_label+", "+else_label+'\n';
                s+=then_label+":\n";
                if_stmt->GenerateIR(s);
                if(!is_return){
                    s+="  jump "+end_label+'\n';
                }
                else{
                    is_return=false;
                    stmt1_return=true;
                }
                s+=else_label+":\n";
                else_stmt->GenerateIR(s);
                if(!is_return){
                    s+="  jump "+end_label+'\n';
                }
                else{
                    is_return=false;
                    stmt2_return=true;
                }
                if(stmt1_return&&stmt2_return){
                    is_return=true;
                }
                if(!stmt1_return||!stmt2_return){
                    s+=end_label+":\n";
                }
                return "";
            case Kind::While:
                current_while.push(while_num);
                while_entry="%while_entry_"+to_string(while_num);
                while_body="%while_body_"+to_string(while_num);
                end_label="%while_end_"+to_string(while_num);
                while_num++;
                s+="  jump "+while_entry+'\n';
                s+=while_entry+":\n";
                if(exp!=nullptr)
                    value=exp->GenerateIR(s);
                s+="  br "+value+", "+while_body+", "+end_label+'\n';
                s+=while_body+":\n";
                while_stmt->GenerateIR(s);
                if(!is_return){
                    s+="  jump "+while_entry+'\n';
                }
                else{
                    is_return=false;
                }
                s+=end_label+":\n";
                current_while.pop();
                return "";
            case Kind::Break:
            // 先查看break在哪层循环中
                if(current_while.empty()){
                    assert(false);
                }
                find_current_while=current_while.top();
                s+="  jump %while_end_"+to_string(find_current_while)+'\n';
                //再生成一个标签，处理后面的while中语句
                s+="%while_body_"+to_string(find_current_while)+"_break_"+to_string(break_num)+":\n";
                break_num++;
                return "";
            case Kind::Continue:
                if(current_while.empty()){
                    assert(false);
                }
                find_current_while=current_while.top();
                s+="  jump %while_entry_"+to_string(find_current_while)+'\n';
                s+="%while_body_"+to_string(find_current_while)+"_continue_"+to_string(continue_num)+":\n";
                continue_num++;
                return "";
            default:
                return "";
        }
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
        // if (is_return)
        //     return "";
        return to_string(int_const);
    }
};

class ExpAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> orexp;

    void Dump() const override{
        std::cout<<"ExpAST { ";
        orexp->Dump();
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
        // if (is_return)
        //     return "";
        string value;
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
        // if (is_return)
        //     return "";
        string current_val;
        string next_val;
        switch (kind){
            case Kind::Number:
                current_val=number->GenerateIR(s);
                break;
            case Kind::Exp:
                current_val=exp->GenerateIR(s);
                break;
            case Kind::LVal:
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
        // if (is_return)
        //     return "";
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
        // if (is_return)
        //     return "";
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
        // if (is_return)
        //     return "";
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
        // if (is_return)
        //     return "";
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Add:
                current_val_2=addexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Rel:
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
                    s+="  "+next_val+" = ge "+current_val_1+", "+current_val_2+"\n";
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
        // if (is_return)
        //     return "";
        string current_val_1;
        string current_val_2;
        string next_val;
        switch (kind){
            case Kind::Rel:
                current_val_2=relexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Eq:
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
        // if (is_return)
        //     return "";
        string current_val_1;
        string current_val_2;
        string next_val;
        int tmp_val;
        switch (kind){
            case Kind::Eq:
                current_val_2=eqexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::And:
                current_val_1=landexp->GenerateIR(s);
                tmp_val=landexp->compute_exp();
                if(tmp_val==0){
                    return current_val_1;
                }
                current_val_2=eqexp->GenerateIR(s);
                // string tmp_val_1="%"+to_string(val_num++);
                // s+="  "+tmp_val_1+" = ne "+current_val_1+", 0\n";
                // string tmp_val_2="%"+to_string(val_num++);
                // s+="  "+tmp_val_2+" = ne "+current_val_2+", 0\n";
                next_val="%"+to_string(val_num++);
                // s+="  "+next_val+" = and "+tmp_val_1+", "+tmp_val_2+"\n";
                s+="  "+next_val+" = ne "+current_val_2+", 0\n";
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
        // if (is_return)
        //     return "";
        string current_val_1;
        string current_val_2;
        string next_val;
        int tmp_val;
        switch (kind){
            case Kind::And:
                current_val_2=landexp->GenerateIR(s);
                next_val=current_val_2;
                break;
            case Kind::Or:
                current_val_1=lorexp->GenerateIR(s);
                tmp_val=lorexp->compute_exp();
                if(tmp_val!=0){
                    next_val="%"+to_string(val_num++);
                    s+="  "+next_val+" = ne "+current_val_1+", 0\n";
                    return next_val;
                }
                current_val_2=landexp->GenerateIR(s);
                // string tmp_val_1="%"+to_string(val_num++);
                // s+="  "+tmp_val_1+" = or "+current_val_1+", "+current_val_2+"\n";
                next_val="%"+to_string(val_num++);
                s+="  "+next_val+" = ne "+current_val_2+", 0\n";
                break;
        }
        return next_val;
    }
};

class DeclAST:public BaseAST{
public:
    enum class Kind{Const,Var};
    Kind kind;
    std::unique_ptr<BaseAST> constdecl;
    std::unique_ptr<BaseAST> vardecl;

    void Dump() const override{
        std::cout<<"DeclAST { ";
        switch (kind){
            case Kind::Const:
                constdecl->Dump();
                break;
            case Kind::Var:
                vardecl->Dump();
                break;
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        switch (kind){
            case Kind::Const:
                constdecl->set_symbol_table(symbol_table);
                break;
            case Kind::Var:
                vardecl->set_symbol_table(symbol_table);
                break;
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        switch (kind){
            case Kind::Const:
                constdecl->GenerateIR(s);
                break;
            case Kind::Var:
                vardecl->GenerateIR(s);
                break;
        }
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
        // if (is_return)
        //     return "";
        for (const auto& c_def:constdef){
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
        // if (is_return)
        //     return "";
        symbol_table->Insert(ident,constinitval->compute_exp());
        return "";
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
        // if (is_return)
        //     return "";
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

    string get_ident() const override{
        return ident;
    }

    int compute_exp() const override{
        if(symbol_table->isExist(ident)){
            std::variant<int,string> value=symbol_table->query(ident);
            if(std::holds_alternative<int>(value)){
                return get<int>(value);
            }
            else if(std::holds_alternative<string>(value)){
                // return symbol_table->var_table[ident];
                return symbol_table->get_val(ident);
            }
        }
        assert(false);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        if(symbol_table->isExist(ident)){
            variant<int,string> value=symbol_table->query(ident);
            if (std::holds_alternative<int>(value)){
                return to_string(get<int>(value));
            }
            else if (std::holds_alternative<string>(value)){
                string current_val="%"+to_string(val_num++);
                s+="  "+current_val+" = load "+get<string>(value)+'\n';
                return current_val;
            }
        }
       assert(false);
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
        // if (is_return)
        //     return "";
        exp->GenerateIR(s);
        return "";
    }
};

class VarDeclAST:public BaseAST{
public:
    std::string btype;
    std::vector<std::unique_ptr<BaseAST>> vardef;

    void Dump() const override{
        std::cout<<"VarDeclAST { "<<btype;
        for (const auto& v_def:vardef){
            v_def->Dump();
            std::cout<<", ";
        }
        std::cout<<" }";
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        for (const auto& v_def:vardef){
            v_def->set_symbol_table(symbol_table);
        }
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        for (const auto& v_def:vardef){
            v_def->GenerateIR(s);
        }
        return "";
    }
};

class VarDefAST:public BaseAST{
public:
    enum class Kind{Ident,Init};
    Kind kind;
    std::string ident;
    std::unique_ptr<BaseAST> initval;

    void Dump() const override{
        std::cout<<"VarDefAST { ";
        switch (kind){
            case Kind::Ident:
                std::cout<<ident;
                break;
            case Kind::Init:
                std::cout<<ident<<", ";
                initval->Dump();
                break;
        }
        std::cout<<" }";
    }

    int compute_exp() const override{
        int value=0;
        if (kind==Kind::Init){
            value=initval->compute_exp();
            symbol_table->var_table[ident]=value;
            return value;
        }
        else{
            assert(false);
        }
    }

    void set_symbol_table(SymbolTable* table) override{
        symbol_table=table;
        if (kind==Kind::Init)
            initval->set_symbol_table(symbol_table);
    }

    string GenerateIR(string& s) const override{
        // if (is_return)
        //     return "";
        string name="@"+ident;
        if(var_count.find(ident)==var_count.end()){
            var_count[ident]=1;
            name+="_"+to_string(var_count[ident]);
        }
        else{
            var_count[ident]++;
            name+="_"+to_string(var_count[ident]);
        }
        symbol_table->Insert(ident,name);
        s+="  "+name+" = alloc i32\n";
        if (kind==Kind::Init){
            symbol_table->var_table[ident]=initval->compute_exp();
            string value=initval->GenerateIR(s);
            s+="  store "+value+", "+name+'\n';
        }
        return "";
    }
};

class InitValAST:public BaseAST{
public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override{
        std::cout<<"InitValAST { ";
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
        // if (is_return)
        //     return "";
        return exp->GenerateIR(s);;
    }
};