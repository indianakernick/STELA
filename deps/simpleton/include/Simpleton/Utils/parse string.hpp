//
//  parse string.hpp
//  Simpleton Engine
//
//  Created by Indi Kernick on 30/9/17.
//  Copyright © 2017 Indi Kernick. All rights reserved.
//

#ifndef engine_utils_parse_string_hpp
#define engine_utils_parse_string_hpp

#include <cctype>
#include <string_view>
#include <type_traits>

namespace Utils {
  ///Keeps track of lines and columns in text.
  ///Great for writing error messages in parsers
  template <
    typename Line_ = uint32_t,
    typename Col_ = uint32_t,
    Col_ SIZE_OF_TAB_ = 8,
    Line_ FIRST_LINE_ = 1,
    Col_ FIRST_COL_ = 1
  >
  class LineCol {
  public:
    using Line = Line_;
    using Col = Col_;
    
    static constexpr Col  SIZE_OF_TAB = SIZE_OF_TAB_;
    static constexpr Line FIRST_LINE  = FIRST_LINE_;
    static constexpr Col  FIRST_COL   = FIRST_COL_;
    
    // 3.9 lines and 18.4 columns doesn't make any sense!
    static_assert(std::is_integral_v<Line>, "Type of line must be an integer");
    static_assert(std::is_integral_v<Col>, "Type of column must be an integer");
  
    LineCol()
      : mLine(FIRST_LINE), mCol(FIRST_COL) {}
    LineCol(const Line line, const Col col)
      : mLine(line), mCol(col) {
      assert(line >= FIRST_LINE);
      assert(col >= FIRST_COL);
    }
    
    ///Move line and col according to the char
    void putChar(const char c) {
      switch (c) {
        case '\t':
          //assumes that Col is an integer
          mCol = (mCol + SIZE_OF_TAB - 1) / SIZE_OF_TAB * SIZE_OF_TAB;
          break;
        case '\n':
          ++mLine;
          mCol = FIRST_COL;
          break;
        case '\v':
        case '\f':
          ++mLine;
          break;
        case '\r':
          mCol = FIRST_COL;
          break;
        case '\b':
          if (mCol != FIRST_COL) {
            --mCol;
          }
          break;
        
        default:
          if (std::isprint(c)) {
            ++mCol;
          }
      }
    }
    ///Call putChar(char) for the first n chars in the string
    void putString(const char *str, size_t size) {
      assert(str);
      while (size) {
        putChar(*str);
        ++str;
        --size;
      }
    }
    
    Line line() const {
      return mLine;
    }
    Col col() const {
      return mCol;
    }
    
  private:
    Line mLine;
    Col mCol;
  };

  ///A view onto a string being parsed
  class ParseString {
  public:
    explicit ParseString(std::string_view);
    ParseString(const char *, size_t);
    
    ///Get the line and column position of the string yet to be parsed
    ///relative to the beginning.
    LineCol<> lineCol() const;
    ///Get a std::string_view of the unparsed string
    std::string_view view() const;
    
    ///Return true if the string is empty
    bool empty() const;
    
    ///Move the front of the string forward. Increments line and column
    ///numbers accordingly
    ParseString &advance(size_t);
    ///Move the front of the string forward by one character. Increments line
    ///and column numbers accordingly
    ParseString &advance();
    
    ///Move the front forward while the supplied predicate returns true
    template <typename Pred>
    ParseString &skip(Pred &&);
    ///Move the front forward while the front is whitespace
    ParseString &skipWhitespace();
    
    ///Move the front forward until the front is equal to the supplied character
    ParseString &skipUntil(char);
    ///Move the front forward until the supplied predicate returns true
    template <typename Pred>
    ParseString &skipUntil(Pred &&);
    
    ///Advances and returns true if the front character is equal to the supplied
    ///character. Does nothing and returns false otherwise
    bool check(char);
    ///Advances and returns true if the front part of the string is equal to the
    ///supplied string. Does nothing and returns false otherwise
    bool check(const char *, size_t);
    ///Advances and returns true if the front part of the string is equal to the
    ///supplied string. Does nothing and returns false otherwise
    bool check(std::string_view);
    ///Advances and returns true if the front part of the string is equal to the
    ///supplied string. Does nothing and returns false otherwise
    template <size_t SIZE>
    bool check(const char (&)[SIZE]);
    ///Advances and returns true if the supplied predicate returns true for the
    ///first character. Does nothing and returns false otherwise
    template <typename Pred>
    bool check(Pred &&);
    
    ///Interprets the front part of the string as an enum. Returns the index of
    ///a name that matches or returns the number of names if no name matches.
    size_t tryParseEnum(const std::string_view *, size_t);
    ///Interprets the front part of the string as an enum followed by a
    ///character for which the predicate returns true. Returns the index of
    ///a name that matches or returns the number of names if no name matches.
    template <typename Pred>
    size_t tryParseEnum(const std::string_view *, size_t, Pred &&);
    
    ///Creates a std::string_view that views the front of the string with a
    ///size of 0
    std::string_view beginViewing() const;
    ///Modify a std::string_view to view a region of the string between the
    ///call to beginViewing and the current position (not including the current
    ///character)
    void endViewing(std::string_view &) const;
    
  private:
    const char *beg;
    const char *end;
    LineCol<> pos;
    
    //Advance without range checks
    void advanceNoCheck(size_t);
    void advanceNoCheck();
  };
}

#include "parse string.inl"

#endif
