#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include "koopa.h"
#include "AST.hpp"
#include <map>
using namespace std;
typedef struct{
  int tag;
}reg_t;

string reg_name[]={"t0","t1","t2","t3","t4","t5","t6","a0","a1","a2","a3","a4","a5","a6","a7","x0"};
map<koopa_raw_value_t,reg_t> reg_map;

int reg_states[16]={0};

reg_t get_reg(){
  reg_t reg;
  for(int i=0;i<15;++i){
    if(reg_states[i]==0){
      reg_states[i]=1;
      reg.tag=i;
      return reg;
    }
  }
  assert(false);
}

void Visit(const koopa_raw_program_t& program,string& s);
void Visit(const koopa_raw_slice_t& slice,string& s);
void Visit(const koopa_raw_function_t& func,string& s);
void Visit(const koopa_raw_basic_block_t& bb,string& s);
reg_t Visit(const koopa_raw_value_t& value,string& s);
reg_t Visit(const koopa_raw_value_t& value,map<koopa_raw_value_t,int>& arg_map,int Stacksize,string& s); 
void Visit(const koopa_raw_return_t& ret,map<koopa_raw_value_t,int>& arg_map,int Stacksize,string& s);
reg_t Visit(const koopa_raw_integer_t& integer,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_binary_t& binary,map<koopa_raw_value_t,int>& arg_map,int offset,string& s);
reg_t Visit(const koopa_raw_aggregate_t& aggregate,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_func_arg_ref_t& func_arg_ref,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_block_arg_ref_t& block_arg_ref,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_global_alloc_t& global_alloc,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_load_t& load,map<koopa_raw_value_t,int>& arg_map,int offset,string& s);
reg_t Visit(const koopa_raw_store_t& store,map<koopa_raw_value_t,int>& arg_map,int Stacksize,string& s);
reg_t Visit(const koopa_raw_get_ptr_t& get_ptr,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_get_elem_ptr_t& get_elem_ptr,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_branch_t& branch,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_jump_t& jump,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_call_t& call,map<koopa_raw_value_t,int>& arg_map,string& s);

static int callee_param_on_stack = 0;

void Visit(const koopa_raw_program_t& program,string& s){
  Visit(program.values,s);
  Visit(program.funcs,s);
}

void Visit(const koopa_raw_slice_t& slice,string& s){
//   switch (slice.kind){
//     case KOOPA_RSIK_FUNCTION:
//       break;
//     case KOOPA_RSIK_VALUE:
//       break;
//     default:break;
//       //assert(false);
//   }
  for(size_t i=0;i<slice.len;++i){
    auto ptr=slice.buffer[i];
    switch (slice.kind){
      case KOOPA_RSIK_FUNCTION:
        if(reinterpret_cast<koopa_raw_function_t>(ptr)->bbs.len!=0){
          Visit(reinterpret_cast<koopa_raw_function_t>(ptr),s);
        }
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr),s);
        break;
      case KOOPA_RSIK_VALUE:
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr),s);
        s+="\n";
        break;
      default:
        assert(false);
    }
  }
}

void Visit(const koopa_raw_function_t& func,string& s){
  callee_param_on_stack = 0;
  s+="  .text\n";
  s+="  .globl ";
  string func_name=func->name;
  if(func_name[0]=='@')
    func_name=func_name.substr(1);
  s+=func_name;
  s+="\n";
  s+=func_name;
  s+=":\n";
  int StackSize=0;
  map<koopa_raw_value_t,int> arg_map;
  for(size_t i=0;i<func->bbs.len;++i){
    auto bb=reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
    for(size_t j=0;j<bb->insts.len;++j){
      auto inst=reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
      if(inst->ty->tag!=KOOPA_RTT_UNIT||inst->kind.tag==KOOPA_RVT_ALLOC){
        arg_map[inst]=StackSize;
        StackSize+=4;
      }
      if(inst->kind.tag==KOOPA_RVT_CALL){
        auto call=inst->kind.data.call;
        if(call.args.len>8){
          if((call.args.len-8)>callee_param_on_stack)
            callee_param_on_stack = call.args.len-8;
        }
      }
    }
  }
  StackSize+=4*callee_param_on_stack;
  StackSize+=4;//return address
  StackSize+=16-StackSize%16;
  if(StackSize<=2048)
    s+="  addi sp, sp, -"+to_string(StackSize)+"\n";
  else{
    reg_t reg=get_reg();
    s+="  li "+reg_name[reg.tag]+", "+to_string(StackSize)+"\n";
    s+="  sub sp, sp, t0\n";
    reg_states[reg.tag]=0;
  }
  s+="  sw ra, ";
  s+=to_string(StackSize-4);
  s+="(sp)\n";
  for(size_t i=0;i<func->bbs.len;++i){
    auto bb=reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
    if(i!=0){
      s+=string(bb->name).substr(1)+":\n";
    }
    for(size_t j=0;j<bb->insts.len;++j){
      auto inst=reinterpret_cast<koopa_raw_value_t>(bb->insts.buffer[j]);
      Visit(inst,arg_map,StackSize,s);
    }
  }
  s+="\n";
}

void Visit(const koopa_raw_basic_block_t& bb,string& s){
  Visit(bb->insts,s);
}

reg_t Visit(const koopa_raw_value_t& value,string& s){
  assert(value->kind.tag==KOOPA_RVT_GLOBAL_ALLOC);
  auto global_alloc=value->kind.data.global_alloc;
  assert(global_alloc.init!=nullptr);
  s+="  .data\n";
  s+="  .globl ";
  string name=value->name;
  if(name[0]=='@')
    name=name.substr(1);
  s+=name+"\n";
  s+=name+":\n";
  if(global_alloc.init->kind.tag==KOOPA_RVT_ZERO_INIT){
    s+="  .zero ";
    if(global_alloc.init->ty->tag==KOOPA_RTT_INT32){
      s+="4\n";
    }
    else{
      assert(false);
    }
  }
  else if(global_alloc.init->kind.tag==KOOPA_RVT_INTEGER){
    s+="  .word ";
    s+=to_string(global_alloc.init->kind.data.integer.value);
    s+="\n\n";
  }
  else{
    assert(false);
  }
  reg_t reg;
  return reg;
}

reg_t Visit(const koopa_raw_value_t& value,map<koopa_raw_value_t,int>& arg_map,int Stacksize,string& s){
  // if (reg_map.find(value)!=reg_map.end())
  //   return reg_map[value];
  reg_t reg;
  const auto& kind=value->kind;
  switch (kind.tag){
    case KOOPA_RVT_RETURN:
      Visit(kind.data.ret,arg_map,Stacksize,s);
      break;
    case KOOPA_RVT_INTEGER:
      reg=Visit(kind.data.integer,arg_map,s);
      break;
    case KOOPA_RVT_BINARY:
      reg=Visit(kind.data.binary,arg_map,arg_map[value],s);
      // reg_map[value]=reg;
      break;
    case KOOPA_RVT_STORE:
      reg=Visit(kind.data.store,arg_map,Stacksize,s);
      break;
    case KOOPA_RVT_LOAD:
      reg=Visit(kind.data.load,arg_map,arg_map[value],s);
      break;
    case KOOPA_RVT_ALLOC:
      break;
    case KOOPA_RVT_JUMP:
      reg=Visit(kind.data.jump,arg_map,s);
      break;
    case KOOPA_RVT_BRANCH:
      reg=Visit(kind.data.branch,arg_map,s);
      break;
    case KOOPA_RVT_CALL:
      reg=Visit(kind.data.call,arg_map,s);
      if(value->ty->tag!=KOOPA_RTT_UNIT){
        s+="  sw a0, "+to_string(arg_map[value]+4*callee_param_on_stack)+"(sp)\n";
      }
      break;
    default:
      assert(false);
  }
  return reg;
}

void Visit(const koopa_raw_return_t& ret,map<koopa_raw_value_t,int>& arg_map,int Stacksize,string& s){
  koopa_raw_value_t ret_value=ret.value;
  if(ret_value==nullptr){
    s+="  lw ra, "+to_string(Stacksize-4)+"(sp)\n";
    if(Stacksize<=2048)
      s+="  addi sp, sp, "+to_string(Stacksize)+"\n";
    else{
      reg_t reg=get_reg();
      s+="  li "+reg_name[reg.tag]+", "+to_string(Stacksize)+"\n";
      s+="  add sp, sp, "+reg_name[reg.tag]+"\n";
      reg_states[reg.tag]=0;
    }
    s+="  ret\n";
    return;
  }
  else if (ret_value->kind.tag==KOOPA_RVT_INTEGER){
    s+="  li a0, "+to_string(ret_value->kind.data.integer.value)+"\n";
    s+="  lw ra, "+to_string(Stacksize-4)+"(sp)\n";
    if(Stacksize<=2048)
      s+="  addi sp, sp, "+to_string(Stacksize)+"\n";
    else{
      reg_t reg=get_reg();
      s+="  li "+reg_name[reg.tag]+", "+to_string(Stacksize)+"\n";
      s+="  add sp, sp, "+reg_name[reg.tag]+"\n";
      reg_states[reg.tag]=0;
    }
    s+="  ret\n";
    return;
  }
//   reg_t reg=Visit(ret_value,arg_map,0,s);
//   string regname=reg_name[reg.tag];
//   s+="  mv a0, "+regname+"\n";
  //此处应该考虑ret_value
  int bias=arg_map[ret_value]+4*callee_param_on_stack;
  s+="  lw a0, "+to_string(bias)+"(sp)\n";
  s+="  lw ra, "+to_string(Stacksize-4)+"(sp)\n";
  if(Stacksize<=2048)
    s+="  addi sp, sp, "+to_string(Stacksize)+"\n";
  else{
    reg_t reg=get_reg();
    s+="  li "+reg_name[reg.tag]+", "+to_string(Stacksize)+"\n";
    s+="  add sp, sp, "+reg_name[reg.tag]+"\n";
    reg_states[reg.tag]=0;
  }
  s+="  ret\n";
  return;
}

reg_t Visit(const koopa_raw_integer_t& integer,map<koopa_raw_value_t,int>& arg_map,string& s){
  int value=integer.value;
  if (value==0){
    reg_t reg={15};
    return reg;
  }
  reg_t reg=get_reg();
  string regname=reg_name[reg.tag];
  s+="  li "+regname+", "+to_string(value)+"\n";
  
  return reg;
}

reg_t Visit(const koopa_raw_binary_t& binary,map<koopa_raw_value_t,int>& arg_map,int offset,string& s){
  reg_t reg1;
  reg_t reg2;
  int bias;
  if(arg_map.find(binary.lhs)!=arg_map.end()){
    bias=arg_map[binary.lhs]+4*callee_param_on_stack;
    reg1=get_reg();
    s+="  lw "+reg_name[reg1.tag]+", "+to_string(bias)+"(sp)\n";
  }
  else
    reg1=Visit(binary.lhs,arg_map,0,s);
  if(arg_map.find(binary.rhs)!=arg_map.end()){
    bias=arg_map[binary.rhs]+4*callee_param_on_stack;
    reg2=get_reg();
    s+="  lw "+reg_name[reg2.tag]+", "+to_string(bias)+"(sp)\n";
  }
  else
    reg2=Visit(binary.rhs,arg_map,0,s);
  string leftname=reg_name[reg1.tag];
  string rightname=reg_name[reg2.tag];
  reg_states[reg1.tag]=0;
  reg_states[reg2.tag]=0;
  reg_t reg=get_reg();
  string resultname=reg_name[reg.tag];
  switch (binary.op){
    case KOOPA_RBO_NOT_EQ:
      s+="  xor "+resultname+", "+leftname+", "+rightname+"\n";
      s+="  snez "+resultname+", "+resultname+"\n";
      break;
    case KOOPA_RBO_EQ:
      s+="  xor "+resultname+", "+leftname+", "+rightname+"\n";
      s+="  seqz "+resultname+", "+resultname+"\n";
      break;
    case KOOPA_RBO_GT:
      s+="  sgt "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_LT:
      s+="  slt "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_GE:
      s+="  slt "+resultname+", "+leftname+", "+rightname+"\n";
      s+="  xori "+resultname+", "+resultname+", 1\n";
      break;
    case KOOPA_RBO_LE:
      s+="  sgt "+resultname+", "+leftname+", "+rightname+"\n";
      s+="  xori "+resultname+", "+resultname+", 1\n";
      break;
    case KOOPA_RBO_ADD:
      s+="  add "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_SUB:
      s+="  sub "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_MUL:
      s+="  mul "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_DIV:
      s+="  div "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_MOD:
      s+="  rem "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_AND:
      s+="  and "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_OR:  
      s+="  or "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_XOR:
      s+="  xor "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_SHL:
      s+="  sll "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_SHR:
      s+="  srl "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    case KOOPA_RBO_SAR:
      s+="  sra "+resultname+", "+leftname+", "+rightname+"\n";
      break;
    default:
      assert(false);
  }
  s+="  sw "+resultname+", "+to_string(offset+4*callee_param_on_stack)+"(sp)\n";
  reg_states[reg.tag]=0;
  return reg;
}

reg_t Visit(const koopa_raw_aggregate_t& aggregate,map<koopa_raw_value_t,int>& arg_map,string& s){
  assert(false);
}

reg_t Visit(const koopa_raw_func_arg_ref_t& func_arg_ref,map<koopa_raw_value_t,int>& arg_map,string& s){
  assert(false);
}

reg_t Visit(const koopa_raw_block_arg_ref_t& block_arg_ref,map<koopa_raw_value_t,int>& arg_map,string& s){
  assert(false);
}

reg_t Visit(const koopa_raw_global_alloc_t& global_alloc,map<koopa_raw_value_t,int>& arg_map,string& s){
  assert(false);
}

reg_t Visit(const koopa_raw_load_t& load,map<koopa_raw_value_t,int>& arg_map,int offset,string& s){
  reg_t reg;
  if(load.src->kind.tag==KOOPA_RVT_GLOBAL_ALLOC){
    string name=load.src->name;
    if(name[0]=='@')
      name=name.substr(1);
    reg=get_reg();
    string regname=reg_name[reg.tag];
    s+="  la "+regname+", "+name+"\n";
    s+="  lw "+regname+", 0("+regname+")\n";
    s+="  sw "+regname+", "+to_string(offset+4*callee_param_on_stack)+"(sp)\n";
    reg_states[reg.tag]=0;
    return reg;
  }
  reg=get_reg();
  string regname=reg_name[reg.tag];
    int bias=arg_map[load.src]+4*callee_param_on_stack;
    s+="  lw "+regname+", "+to_string(bias)+"(sp)\n";
    s+="  sw "+regname+", "+to_string(offset+4*callee_param_on_stack)+"(sp)\n";
    reg_states[reg.tag]=0;
    return reg;
}

reg_t Visit(const koopa_raw_store_t& store,map<koopa_raw_value_t,int>& arg_map,int Stacksize,string& s){
  reg_t reg;
  reg=get_reg();
  string regname=reg_name[reg.tag];
  if(store.value->kind.tag==KOOPA_RVT_INTEGER){
    int num=store.value->kind.data.integer.value;
    s+="  li "+regname+", "+to_string(num)+"\n";
  }
  else if(store.value->kind.tag==KOOPA_RVT_FUNC_ARG_REF){
    size_t index=store.value->kind.data.func_arg_ref.index;
    if(index<8){
      s+="  mv "+regname+", a"+to_string(index)+"\n";
    }
    else{
      int bias=4*(index-8)+Stacksize;
      s+="  lw "+regname+", "+to_string(bias)+"(sp)\n";
    }
  }
  else{
    int bias=arg_map[store.value]+4*callee_param_on_stack;
    s+="  lw "+regname+", "+to_string(bias)+"(sp)\n";
  }
  if(store.dest->kind.tag==KOOPA_RVT_GLOBAL_ALLOC){
    string name=store.dest->name;
    if(name[0]=='@')
      name=name.substr(1);
    reg_t dest_reg=get_reg();
    string dest_regname=reg_name[dest_reg.tag];
    s+="  la "+dest_regname+", "+name+"\n";
    s+="  sw "+regname+", 0("+dest_regname+")\n";
    reg_states[reg.tag]=0;
    reg_states[dest_reg.tag]=0;
    return reg;
  }
  else{
    s+="  sw "+regname+", "+to_string(arg_map[store.dest]+4*callee_param_on_stack)+"(sp)\n";
    reg_states[reg.tag]=0;
    return reg;
  }
}

reg_t Visit(const koopa_raw_get_ptr_t& get_ptr,map<koopa_raw_value_t,int>& arg_map,string& s){
  assert(false);
}

reg_t Visit(const koopa_raw_get_elem_ptr_t& get_elem_ptr,map<koopa_raw_value_t,int>& arg_map,string& s){
  assert(false);
}

reg_t Visit(const koopa_raw_branch_t& branch,map<koopa_raw_value_t,int>& arg_map,string& s){
  reg_t reg;
  reg=get_reg();
  string regname=reg_name[reg.tag];
  if(branch.cond->kind.tag==KOOPA_RVT_INTEGER){
    int num=branch.cond->kind.data.integer.value;
    s+="  li "+regname+", "+to_string(num)+"\n";
  }
  else{
    int bias=arg_map[branch.cond]+4*callee_param_on_stack;
    s+="  lw "+regname+", "+to_string(bias)+"(sp)\n";
  }
  s+="  bnez "+regname+", "+string(branch.true_bb->name).substr(1)+"\n";
  s+="  j "+string(branch.false_bb->name).substr(1)+"\n";
  reg_states[reg.tag]=0;
  return reg;
}

reg_t Visit(const koopa_raw_jump_t& jump,map<koopa_raw_value_t,int>& arg_map,string& s){
  s+="  j "+string(jump.target->name).substr(1)+"\n";
  reg_t reg;
  return reg;
}

reg_t Visit(const koopa_raw_call_t& call,map<koopa_raw_value_t,int>& arg_map,string& s){
  int arg_num=call.args.len;
  for(int i=0;i<arg_num;++i){
    auto arg=reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
    if(arg->kind.tag==KOOPA_RVT_INTEGER){
      if(i<8){
        s+="  li a"+to_string(i)+", "+to_string(arg->kind.data.integer.value)+"\n";
      }
      else{
        int bias=4*(i-8);
        s+="  li t0, "+to_string(arg->kind.data.integer.value)+"\n";
        s+="  sw t0, "+to_string(bias)+"(sp)\n";
      }
    }
    else{
      int bias=arg_map[arg]+4*callee_param_on_stack;
      if(i<8){
        s+="  lw a"+to_string(i)+", "+to_string(bias)+"(sp)\n";
      }
      else{
        int offset=4*(i-8);
        s+="  lw t0, "+to_string(bias)+"(sp)\n";
        s+="  sw t0, "+to_string(offset)+"(sp)\n";
      }
    }
  }
  string func_name=call.callee->name;
  if(func_name[0]=='@')
    func_name=func_name.substr(1);
  s+="  call "+func_name+"\n";
  reg_t reg;
  return reg;
}