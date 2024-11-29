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
void Visit(const koopa_raw_return_t& ret,string& s);
reg_t Visit(const koopa_raw_integer_t& integer,string& s);
reg_t Visit(const koopa_raw_binary_t& binary,string& s);

void Visit(const koopa_raw_program_t& program,string& s){

  Visit(program.values,s);
  Visit(program.funcs,s);
}

void Visit(const koopa_raw_slice_t& slice,string& s){
  switch (slice.kind){
    case KOOPA_RSIK_FUNCTION:
      s+="  .text\n";
      break;
    case KOOPA_RSIK_VALUE:
      break;
    default:break;
      //assert(false);
  }
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
  Visit(func->bbs,s);
}

void Visit(const koopa_raw_basic_block_t& bb,string& s){
  Visit(bb->insts,s);
}

reg_t Visit(const koopa_raw_value_t& value,string& s){
  if (reg_map.find(value)!=reg_map.end())
    return reg_map[value];
  reg_t reg;
  const auto& kind=value->kind;
  switch (kind.tag){
    case KOOPA_RVT_RETURN:
      Visit(kind.data.ret,s);
      break;
    case KOOPA_RVT_INTEGER:
      reg=Visit(kind.data.integer,s);
      break;
    case KOOPA_RVT_BINARY:
      reg=Visit(kind.data.binary,s);
      reg_map[value]=reg;
      break;
    default:
      assert(false);
  }
  return reg;
}

void Visit(const koopa_raw_return_t& ret,string& s){
  koopa_raw_value_t ret_value=ret.value;
  if (ret_value->kind.tag==KOOPA_RVT_INTEGER){
    s+="  li a0, "+to_string(ret_value->kind.data.integer.value)+"\n";
    s+="  ret\n";
    return;
  }
  reg_t reg=Visit(ret_value,s);
  string regname=reg_name[reg.tag];
  s+="  mv a0, "+regname+"\n";
  s+="  ret\n";
  return;
}

reg_t Visit(const koopa_raw_integer_t& integer,string& s){
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

reg_t Visit(const koopa_raw_binary_t& binary,string& s){
  reg_t reg1=Visit(binary.lhs,s);
  reg_t reg2=Visit(binary.rhs,s);
  reg_t reg=get_reg();
  string leftname=reg_name[reg1.tag];
  string rightname=reg_name[reg2.tag];
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
  reg_states[reg1.tag]=0;
  reg_states[reg2.tag]=0;
  return reg;
}