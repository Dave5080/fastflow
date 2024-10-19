#include <stdlib.h>
#include <cstdint>
#include <map>
#include <optional>
#include "lexer.h"
typedef std::map<std::string, std::vector<size_t>> FF_AFF_SETS;
typedef std::map<std::string, cpu_set_t> FF_AFF_CPU_SETS;

class Parser{
  public:
    Parser(Lexer& lex) noexcept : lex(lex) {}
    Parser(std::string str) noexcept : lex(str.c_str()){}

    std::optional<FF_AFF_SETS> parse() noexcept;
    std::optional<FF_AFF_CPU_SETS> parse_set() noexcept;
    std::optional<FF_AFF_SETS> parse_func() noexcept;
    std::optional<FF_AFF_SETS> parse_set_list() noexcept;
    std::vector<size_t> parse_cores() noexcept;

  private:
    Lexer lex;

    std::string hnk(std::vector<size_t> v) noexcept {
      std::size_t ret = 0;
      for(auto& i : v) {
        ret ^= std::hash<size_t>()(i);
      }
      return std::to_string(ret);
    }

};

std::optional<FF_AFF_SETS> ff_func_exec(std::string f_name, int arg){
  if(arg < 0)
    return std::nullopt;
  std::optional<FF_AFF_SETS> list = std::make_optional<FF_AFF_SETS>();
  size_t places = 0, p_size = 0;
  if(f_name == "threads"){
      places = ff_numCores();
      p_size = 1;
  } else if(f_name == "cores"){
      places = ff_realNumCores();
      p_size = ff_numCores() / places;
  } else if(f_name == "sockets"){
      places = ff_numSockets();
      places = ff_numCores() / places;
  } else
      return std::nullopt;
  for(size_t i = 0; (arg == 0 && i < places) || (arg > 0 && i <= arg); i++){
    std::vector<size_t> p;
    for(size_t j = 0; j < p_size; j++)
      p.push_back(i*p_size + j);
    (*list)[std::to_string(i)] = p;
  }
  return list;
} 

std::optional<FF_AFF_SETS> ff_func_exec(std::string fname){
  return ff_func_exec(fname, 0);
}

std::optional<FF_AFF_SETS> Parser::parse() noexcept {
  Token tok = lex.peek();
  if(tok.is(Token::Type::NAME)) return parse_func();
  if(tok.is_one_of(Token::Type::HASH, Token::Type::LEFT_SB, Token::Type::NOT)) return parse_set_list();
  return std::nullopt;
}

std::optional<FF_AFF_SETS> Parser::parse_func() noexcept {
  Token tok = lex.next();
  if(tok.is(Token::Type::NAME)){
    std::string f_name = tok.lexme_string();
    tok = lex.next();
    if(tok.is(Token::Type::END)) return ff_func_exec(f_name);
    if(tok.is_not(Token::Type::LEFT_CB)) return std::nullopt;
    Token arg_tok = lex.next();
    if(arg_tok.is_not(Token::Type::NUM)) return std::nullopt;
    std::optional<int> arg_o = arg_tok.lexme_int();
    if(!arg_o) return std::nullopt;
    int arg = *arg_o;
    tok = lex.next();
    if(tok.is_not(Token::Type::RIGHT_CB)) return std::nullopt;
    return ff_func_exec(f_name, arg);
  }
  return std::nullopt;
}

std::optional<FF_AFF_SETS> Parser::parse_set_list() noexcept {
  FF_AFF_SETS result;
  do {
    Token tok = lex.next();
    bool not_enabled = false, len_enabled = false, has_label = false;
    uint16_t len = 1; 
    int stride = 1;
    std::string label = "\0";
    if(tok.is(Token::Type::HASH)){
      Token lab_tok = lex.next();
      if(lab_tok.is_not(Token::Type::NAME)) return std::nullopt;
      has_label = true;
      label = lab_tok.lexme_string();
      if(lex.next().is_not(Token::Type::HASH));
      tok = lex.next();
    }
    if(tok.is(Token::Type::NOT)){
      not_enabled = true;
      tok = lex.next();
    }
    if(tok.is_not(Token::Type::LEFT_SB)) return std::nullopt;
    std::vector<size_t> next_subset = parse_cores();
    if(next_subset.size() == 0) return std::nullopt;
    tok = lex.next();
    if(tok.is(Token::Type::COLON)){
      if(not_enabled || has_label) return std::nullopt;
      Token len_tok = lex.next();
      if(len_tok.is_not(Token::Type::NUM)) return std::nullopt;
      len_enabled = true;
      std::optional<uint16_t> len_o = len_tok.lexme_uint();
      if(!len_o) return std::nullopt;
      len = *len_o;
      tok = lex.next();
      if(tok.is(Token::Type::COLON)){
        Token str_tok = lex.next();
        if(str_tok.is_not(Token::Type::NUM)) return std::nullopt;
        std::optional<int> stride_o = str_tok.lexme_int();
        if(!stride_o) return std::nullopt;
        stride = *stride_o;
        tok = lex.next();
      }
    }
    
    if(!has_label)
      label = hnk(next_subset);
    if(not_enabled){

    }
    result[label] = next_subset;
    
    for(int i = 1; i < len; i++){
      std::vector<size_t> sub_clone;
      for(int j = 0; j < next_subset.size(); j++)
        sub_clone.push_back(next_subset[j] + stride*i);
      result[hnk(sub_clone)] = sub_clone;
    }
    
    if(tok.is(Token::Type::END)) return std::make_optional<FF_AFF_SETS>(result);
    if(tok.is_not(Token::Type::COMMA)) return std::nullopt; 

  }while(true);
}

std::vector<size_t> Parser::parse_cores() noexcept {
 std::vector<size_t> result;
 do{
  Token tok = lex.next();
  bool not_enabled = false, len_enabled = false;
  uint16_t len = 1;
  int stride = 1;
  if(tok.is(Token::Type::NOT)){
    not_enabled = true;
    tok = lex.next();
  }
  if(tok.is_not(Token::Type::NUM)) return {};
  std::optional<size_t> cpu_o = tok.lexme_uint();
  if(!cpu_o) return {};
  size_t cpu = *cpu_o;
  tok = lex.next();
  if(tok.is(Token::Type::COLON)){
    if(not_enabled) return {};
    Token len_tok = lex.next();
    if(len_tok.is_not(Token::Type::NUM)) return {};
    std::optional<uint16_t> len_o = len_tok.lexme_uint();
    if(!len_o) return {};
    len = *len_o;
    len_enabled = true;
    tok = lex.next();
    if(tok.is(Token::Type::COLON)) {
      Token stride_tok = lex.next();
      if(stride_tok.is_not(Token::Type::NUM)) return {};
      std::optional<int> stride_o = stride_tok.lexme_int();
      if(!stride_o) return {};
      stride = *stride_o;
      tok = lex.next();
    }
  } 
  if(not_enabled){

  }
  result.push_back(cpu);
  if(len_enabled)
    for(int i = 1; i < len; i++){
      result.push_back(cpu + stride*i);
    }

  if(tok.is(Token::Type::COMMA)) continue;
  if(tok.is(Token::Type::RIGHT_SB)) return result;
  return {};
 }while(true);
 return {};
}

std::optional<FF_AFF_CPU_SETS> Parser::parse_set() noexcept {
  auto res = parse();
  if(!res) return std::nullopt;
  FF_AFF_CPU_SETS result;
  for(auto pair : *res){
    cpu_set_t tmp;
    CPU_ZERO(&tmp);
    for(auto cpu : pair.second)
      CPU_SET(cpu, &tmp);
    result[pair.first] = tmp;
  }
  return result;
}
