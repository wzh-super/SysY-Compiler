#include <cassert>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include "koopa.h"
#include "AST.hpp"

using namespace std;

extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

void Visit(const koopa_raw_program_t& program,string& s);
void Visit(const koopa_raw_slice_t& slice,string& s);
void Visit(const koopa_raw_function_t& func,string& s);
void Visit(const koopa_raw_basic_block_t& bb,string& s);
void Visit(const koopa_raw_value_t& value,string& s); 
void Visit(const koopa_raw_return_t& ret,string& s);
void Visit(const koopa_raw_integer_t& integer,string& s);

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

void Visit(const koopa_raw_value_t& value,string& s){
  const auto& kind=value->kind;
  switch (kind.tag){
    case KOOPA_RVT_RETURN:
      Visit(kind.data.ret,s);
      break;
    case KOOPA_RVT_INTEGER:
      Visit(kind.data.integer,s);
      break;
    default:
      assert(false);
  }
}

void Visit(const koopa_raw_return_t& ret,string& s){
  koopa_raw_value_t ret_value=ret.value;
  s+="  li a0, ";
  s+=to_string(ret_value->kind.data.integer.value);
  s+="\n";
  s+="  ret\n";
}

void Visit(const koopa_raw_integer_t& integer,string& s){
  return;
}

int main(int argc, const char *argv[]) {

  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  yyin = fopen(input, "r");
  assert(yyin);

  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  string koopaIR;
  ast->GenerateIR(koopaIR);

  if(strcmp(mode,"-koopa")==0){
    ofstream out(output);
    out << koopaIR;
    out.close();
  }
  else if(strcmp(mode,"-riscv")==0){
    koopa_program_t program;
    koopa_error_code_t ret=koopa_parse_from_string(koopaIR.c_str(),&program);
    assert(ret==KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder=koopa_new_raw_program_builder();
    koopa_raw_program_t raw=koopa_build_raw_program(builder,program);
    koopa_delete_program(program);
    string risc;
    Visit(raw,risc);
    ofstream out(output);
    out << risc;
    out.close();
    koopa_delete_raw_program_builder(builder);

  }
  return 0;
}
