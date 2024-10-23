#include <string>
#include <string_view>
#include <charconv>
#include <cstdio>
#include <vector>
#include <iostream>
#include <optional>
class Token {
  public:
    enum class Type{
      NUM = 0,
      NAME = 1,
      LEFT_CB = 2,
      RIGHT_CB = 3,
      LEFT_SB = 4,
      RIGHT_SB = 5,
      COLON = 6,
      NOT = 7,
      COMMA = 8,
      HASH = 9,
      END = 10,
      ERR = 11,
    };

    Token(Type T) noexcept : m_type(T) {}
    Token(Type T, const char* beg, std::size_t len) noexcept : m_type(T), m_lexme(beg, len) {}
    Token(Type T, const char* beg, const char* end) noexcept : m_type(T), m_lexme(beg, end-beg) {}

    Type type() const noexcept { return m_type; }
    void type(Type T) noexcept { m_type = T; }
    
    bool is(Type T) noexcept { return m_type == T; }
    bool is_not(Type T)  noexcept { return !this->is(T); }
    bool is_one_of(Type T1, Type T2) noexcept { return this->is(T1) || this->is(T2); }
    template<typename... Ts>
    bool is_one_of(Type T1, Type T2, Ts...others) 
                    noexcept { return is(T1) || is_one_of(T2, others...); }

    std::string_view lexme() const noexcept { return m_lexme; }
    std::string lexme_string() const noexcept { return {m_lexme.begin(), m_lexme.end()}; }

    std::optional<int> lexme_int() noexcept {
      int val;
      const auto res = std::from_chars(m_lexme.data(), m_lexme.data() + m_lexme.size(), val);
      return std::make_optional<int>(val);
    }

    std::optional<size_t> lexme_uint() noexcept {
      std::optional<int> res = lexme_int();
      if(!res || *res < 0) return std::nullopt;
      return std::make_optional<unsigned int>((unsigned int) *res);
    } 

    void print(){
      switch(m_type){
        case Type::NUM: std::cout << "NUM "; break;
        case Type::NAME: std::cout << "NAME "; break;
        case Type::LEFT_CB: std::cout << "LEFT_CB "; break;
        case Type::RIGHT_CB: std::cout << "RIGHT_CB "; break;
        case Type::LEFT_SB: std::cout << "LEFT_SB "; break;
        case Type::RIGHT_SB: std::cout << "RIGHT_SB "; break;
        case Type::COLON: std::cout << "COLON "; break;
        case Type::NOT: std::cout << "NOT "; break;
        case Type::COMMA: std::cout << "COMMA "; break;
        case Type::HASH: std::cout << "HASH "; break;
        case Type::END: std::cout << "END "; break;
        default: std::cout << "ERR "; break;
      }
      std::cout << ": " << lexme_string() << std::endl;
    }


    void lexme(std::string_view lexme) noexcept { m_lexme = std::move(lexme); }

  private:
    Type m_type;
    std::string_view m_lexme;

};

class Lexer {
  public:
    Lexer(const char* beg) noexcept : m_beg(beg) {
      //std::cout << "STR: " << beg << std::endl;
    }
    Token next() noexcept;
    std::vector<Token> next(unsigned int k) noexcept;
    Token peek() noexcept;
    std::vector<Token> peek(unsigned int k) noexcept;

  private:
    Token name() noexcept;
    Token num() noexcept;
    Token atom(Token::Type) noexcept;

    char peek_c() { return *m_beg; }
    char get() noexcept { return *m_beg++; }
    const char* m_beg = nullptr;
};



bool is_name_char(char c) noexcept { return ((int) c >= 65 && (int) c <= 90) || ((int) c >= 97 && (int) c <= 122); }
bool is_space(char c) noexcept { return (int) c == ' ' || (int) c == '\n' || (int) c == '\t' || (int) c == '\r'; }
bool is_digit(char c) noexcept { return ((int) c >= 48 && (int) c <= 57) || c == '-'; }

Token Lexer::atom(Token::Type T) noexcept { 
  const char* start = m_beg;
  m_beg++;
  return Token(T, start, 1); 
}

Token Lexer::next() noexcept {
  Token result(Token::Type::ERR, m_beg, 1);
  while(is_space(peek_c())) get();
  
  if(peek_c() == '\0') result = Token(Token::Type::END, m_beg, 1);
  else if(is_name_char(peek_c())) result = name();
  else if(is_digit(peek_c())) result = num();

  else switch(peek_c()){
    case '(': result = atom(Token::Type::LEFT_CB); break;
    case ')': result = atom(Token::Type::RIGHT_CB); break;
    case '[': result = atom(Token::Type::LEFT_SB); break;
    case ']': result = atom(Token::Type::RIGHT_SB); break;
    case ':': result = atom(Token::Type::COLON); break;
    case '!': result = atom(Token::Type::NOT); break;
    case ',': result = atom(Token::Type::COMMA); break;
    case '#': result = atom(Token::Type::HASH); break;
  }
  
//  result.print();
  return result;
}

std::vector<Token> Lexer::next(unsigned int k) noexcept {
  std::vector<Token> t_vec;
  for(; k > 0; k--)
    t_vec.push_back(this->next());
  return t_vec;
}

Token Lexer::peek() noexcept {
  const char* start = m_beg;
  Token val = this->next();
  m_beg = start;
  return val;
}

std::vector<Token> Lexer::peek(unsigned int k) noexcept {
  std::vector<Token> t_vec;
  for(; k < 0; k--)
    t_vec.push_back(this->peek());
  return t_vec;
}

Token Lexer::name() noexcept {
  const char* start = m_beg;
  get();
  while(is_name_char(peek_c())) get();
  Token tok(Token::Type::NAME, start, m_beg);
  return tok;
}

Token Lexer::num() noexcept {
  const char* start = m_beg;
  get();
  while(is_digit(peek_c())) get();
  Token tok(Token::Type::NUM, start, m_beg);
  return tok;
}

