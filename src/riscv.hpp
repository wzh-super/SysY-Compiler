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
reg_t Visit(const koopa_raw_store_t& store,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_get_ptr_t& get_ptr,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_get_elem_ptr_t& get_elem_ptr,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_branch_t& branch,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_jump_t& jump,map<koopa_raw_value_t,int>& arg_map,string& s);
reg_t Visit(const koopa_raw_call_t& call,map<koopa_raw_value_t,int>& arg_map,string& s);

void Visit(const koopa_raw_program_t& program,string& s){

  Visit(program.values,s);
  s+="  .text\n";
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
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr),s);
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr),s);
        break;
      case KOOPA_RSIK_VALUE:
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr),s);
        break;
      default:
        assert(false);
    }
  }
}

void Visit(const koopa_raw_function_t& func,string& s){
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
      if(inst->ty->tag==KOOPA_RTT_INT32||inst->kind.tag==KOOPA_RVT_ALLOC){
        arg_map[inst]=StackSize;
        StackSize+=4;
      }
    }
  }
  StackSize+=16-StackSize%16;
  if(StackSize<=2048)
    s+="  addi sp, sp, -"+to_string(StackSize)+"\n";
  else{
    reg_t reg=get_reg();
    s+="  li "+reg_name[reg.tag]+", "+to_string(StackSize)+"\n";
    s+="  sub sp, sp, t0\n";
    reg_states[reg.tag]=0;
  }
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
}

void Visit(const koopa_raw_basic_block_t& bb,string& s){
  Visit(bb->insts,s);
}

reg_t Visit(const koopa_raw_value_t& value,string& s){
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
      reg=Visit(kind.data.store,arg_map,s);
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
    default:
      assert(false);
  }
  return reg;
}

void Visit(const koopa_raw_return_t& ret,map<koopa_raw_value_t,int>& arg_map,int Stacksize,string& s){
  koopa_raw_value_t ret_value=ret.value;
  if (ret_value->kind.tag==KOOPA_RVT_INTEGER){
    s+="  li a0, "+to_string(ret_value->kind.data.integer.value)+"\n";
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
  int bias=arg_map[ret_value];
  s+="  lw a0, "+to_string(bias)+"(sp)\n";
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
    bias=arg_map[binary.lhs];
    reg1=get_reg();
    s+="  lw "+reg_name[reg1.tag]+", "+to_string(bias)+"(sp)\n";
  }
  else
    reg1=Visit(binary.lhs,arg_map,0,s);
  if(arg_map.find(binary.rhs)!=arg_map.end()){
    bias=arg_map[binary.rhs];
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
  s+="  sw "+resultname+", "+to_string(offset)+"(sp)\n";
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
  reg=get_reg();
  string regname=reg_name[reg.tag];
  int bias=arg_map[load.src];
  s+="  lw "+regname+", "+to_string(bias)+"(sp)\n";
  s+="  sw "+regname+", "+to_string(offset)+"(sp)\n";
  reg_states[reg.tag]=0;
  return reg;
}

reg_t Visit(const koopa_raw_store_t& store,map<koopa_raw_value_t,int>& arg_map,string& s){
  reg_t reg;
  reg=get_reg();
  string regname=reg_name[reg.tag];
  if(store.value->kind.tag==KOOPA_RVT_INTEGER){
    int num=store.value->kind.data.integer.value;
    s+="  li "+regname+", "+to_string(num)+"\n";
  }
  else{
    int bias=arg_map[store.value];
    s+="  lw "+regname+", "+to_string(bias)+"(sp)\n";
  }
  s+="  sw "+regname+", "+to_string(arg_map[store.dest])+"(sp)\n";
  reg_states[reg.tag]=0;
  return reg;
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
    int bias=arg_map[branch.cond];
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
  assert(false);
}