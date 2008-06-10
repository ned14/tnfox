/********************************************************************************
*                                                                               *
*                           S t r i n g   O b j e c t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2006 by Jeroen van der Zijp.   All Rights Reserved.        *
* TnFOX extensions (C) 2003-2006 Niall Douglas                                  *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: FXString.h,v 1.120 2006/02/20 03:32:12 fox Exp $                         *
********************************************************************************/
#ifndef FXSTRING_H
#define FXSTRING_H

#include "fxdefs.h"
#include <stdarg.h>

/*! \file FXString.h
\brief Defines a string object
*/

namespace FX {


/*! \class FXString
\brief Provides essential string manipulation capabilities

This is as FOX but with failure to allocate memory causing the usual TnFOX
memory full exception plus many methods have been marked \c throw() to
enable better optimisation. Furthermore some Qt-compatible APIs have been
added and extra storage used to speed up operations such as inserts.
*/
class FXAPI FXString {
private:
  FXchar *str;
  FXint *inserts;
public:
  static const FXchar null[];
  static const FXchar hex[17];
  static const FXchar HEX[17];
public:
  static const signed char utfBytes[256];
public:

  /// Create empty string
  FXString() throw();

  /// Copy construct
  FXString(const FXString& s);

#ifdef HAVE_CPP0XRVALUEREFS
  /// Move construct
  FXString(FXString && s) : str(s.str), inserts(s.inserts) { s.str=nullStr().str; s.inserts=0; }

  /// Move assignment
  FXString &operator=(FXString && s) { str=s.str; inserts=s.inserts; s.str=nullStr().str; s.inserts=0; return *this; }
#endif

  /// Construct and init from string
  FXString(const FXchar* s);

  /// Construct and init from wide character string
  FXString(const FXwchar* s);

  /// Construct and init from narrow character string
  FXString(const FXnchar* s);

  /// Construct and init with substring
  FXString(const FXchar* s,FXint n);

  /// Construct and init with wide character substring
  FXString(const FXwchar* s,FXint n);

  /// Construct and init with narrow character substring
  FXString(const FXnchar* s,FXint n);

  /// Construct and fill with constant
  FXString(FXchar c,FXint n);

  /// Length of text in bytes
  FXint length() const throw() { return *(((FXint*)str)-1); }

  /// Change the length of the string to len
  void length(FXint len);

  /// Count number of utf8 characters
  FXint count() const throw();

  /// Count number of utf8 characters in subrange
  FXint count(FXint pos,FXint len) const throw();

  /// Return byte offset of utf8 character at index
  FXint offset(FXint indx) const throw();

  /// Return index of utf8 character at byte offset
  FXint index(FXint offs) const throw();

  /// Validate position to point to begin of utf8 character
  FXint validate(FXint p) const throw();

  /// Return extent of utf8 character at position
  FXint extent(FXint p) const throw() { return utfBytes[(FXuchar)str[p]]; }

  /// Return start of next utf8 character
  FXint inc(FXint p) const throw();

  /// Return start of previous utf8 character
  FXint dec(FXint p) const throw();

  /// Get text contents
  const FXchar* text() const throw() { return (const FXchar*)str; }

  /// See if string is empty
  bool empty() const throw() { return (((FXint*)str)[-1]==0); }

  /// See if string is empty
  bool operator!() const throw() { return (((FXint*)str)[-1]==0); }

  /// Return a non-const reference to the ith character
  FXchar& operator[](FXint i) throw() { return str[i]; }

  /// Return a const reference to the ith character
  const FXchar& operator[](FXint i) const throw() { return str[i]; }

  /// Return a non-const reference to the ith character
  FXchar& at(FXint i){ return str[i]; }

  /// Return a const reference to the ith character
  const FXchar& at(FXint i) const { return str[i]; }

  /// Return wide character starting at offset i
  FXwchar wc(FXint i) const throw();

  /// Assign a string to this
  FXString& operator=(const FXchar* s);

  /// Assign a wide character string to this
  FXString& operator=(const FXwchar* s);

  /// Assign a narrow character string to this
  FXString& operator=(const FXnchar* s);

  /// Assign another string to this
  FXString& operator=(const FXString& s);

  /// Fill with a constant
  FXString& fill(FXchar c,FXint n);

  /// Fill up to current length
  FXString& fill(FXchar c) throw();

  /// Convert to lower case
  FXString& lower();

  /// Convert to upper case
  FXString& upper();

  /// Return num partition(s) beginning at start from a string separated by delimiters delim.
  FXString section(FXchar delim,FXint start,FXint num=1) const;

  /// Return num partition(s) beginning at start from a string separated by set of delimiters from delim of size n
  FXString section(const FXchar* delim,FXint n,FXint start,FXint num) const;

  /// Return num partition(s) beginning at start from a string separated by set of delimiters from delim.
  FXString section(const FXchar* delim,FXint start,FXint num=1) const;

  /// Return num partition(s) beginning at start from a string separated by set of delimiters from delim.
  FXString section(const FXString& delim,FXint start,FXint num=1) const;

  /// Adopt string s, leaving s empty
  FXString& adopt(FXString& s) throw();

  /// Assign character c to this string
  FXString& assign(FXchar c);

  /// Assign n characters c to this string
  FXString& assign(FXchar c,FXint n);

  /// Assign first n characters of string s to this string
  FXString& assign(const FXchar *s,FXint n);

  /// Assign first n characters of wide character string s to this string
  FXString& assign(const FXwchar *s,FXint n);

  /// Assign first n characters of narrow character string s to this string
  FXString& assign(const FXnchar *s,FXint n);

  /// Assign string s to this string
  FXString& assign(const FXchar* s);

  /// Assign wide character string s to this string
  FXString& assign(const FXwchar* s);

  /// Assign narrow character string s to this string
  FXString& assign(const FXnchar* s);

  /// Assign string s to this string
  FXString& assign(const FXString& s);

  /// Insert character at specified position
  FXString& insert(FXint pos,FXchar c);

  /// Insert n characters c at specified position
  FXString& insert(FXint pos,FXchar c,FXint n);

  /// Insert first n characters of string at specified position
  FXString& insert(FXint pos,const FXchar* s,FXint n);

  /// Insert first n characters of wide character string at specified position
  FXString& insert(FXint pos,const FXwchar* s,FXint n);

  /// Insert first n characters of narrow character string at specified position
  FXString& insert(FXint pos,const FXnchar* s,FXint n);

  /// Insert string at specified position
  FXString& insert(FXint pos,const FXchar* s);

  /// Insert wide character string at specified position
  FXString& insert(FXint pos,const FXwchar* s);

  /// Insert narrow character string at specified position
  FXString& insert(FXint pos,const FXnchar* s);

  /// Insert string at specified position
  FXString& insert(FXint pos,const FXString& s);

  /// Prepend string with input character
  FXString& prepend(FXchar c);

  /// Prepend string with n characters c
  FXString& prepend(FXchar c,FXint n);

  /// Prepend first n characters of string s
  FXString& prepend(const FXchar* s,FXint n);

  /// Prepend first n characters of wide character string s
  FXString& prepend(const FXwchar* s,FXint n);

  /// Prepend first n characters of narrow character string s
  FXString& prepend(const FXnchar* s,FXint n);

  /// Prepend string with string s
  FXString& prepend(const FXchar* s);

  /// Prepend string with wide character string
  FXString& prepend(const FXwchar* s);

  /// Prepend string with narrow character string
  FXString& prepend(const FXnchar* s);

  /// Prepend string with string s
  FXString& prepend(const FXString& s);

  /// Append character c to this string
  FXString& append(FXchar c);

  /// Append n characters c to this string
  FXString& append(FXchar c,FXint n);

  /// Append first n characters of string s to this string
  FXString& append(const FXchar* s,FXint n);

  /// Append first n characters of wide character string s to this string
  FXString& append(const FXwchar* s,FXint n);

  /// Append first n characters of narrow character string s to this string
  FXString& append(const FXnchar* s,FXint n);

  /// Append string s to this string
  FXString& append(const FXchar* s);

  /// Append wide character string s to this string
  FXString& append(const FXwchar* s);

  /// Append narrow character string s to this string
  FXString& append(const FXnchar* s);

  /// Append string s to this string
  FXString& append(const FXString& s);

  /// Replace a single character
  FXString& replace(FXint pos,FXchar c);

  /// Replace the m characters at pos with n characters c
  FXString& replace(FXint pos,FXint m,FXchar c,FXint n);

  /// Replaces the m characters at pos with first n characters of string s
  FXString& replace(FXint pos,FXint m,const FXchar* s,FXint n);

  /// Replaces the m characters at pos with first n characters of wide character string s
  FXString& replace(FXint pos,FXint m,const FXwchar* s,FXint n);

  /// Replaces the m characters at pos with first n characters of narrow character string s
  FXString& replace(FXint pos,FXint m,const FXnchar* s,FXint n);

  /// Replace the m characters at pos with string s
  FXString& replace(FXint pos,FXint m,const FXchar* s);

  /// Replace the m characters at pos with wide character string s
  FXString& replace(FXint pos,FXint m,const FXwchar* s);

  /// Replace the m characters at pos with narrow character string s
  FXString& replace(FXint pos,FXint m,const FXnchar* s);

  /// Replace the m characters at pos with string s
  FXString& replace(FXint pos,FXint m,const FXString& s);

  /// Move range of m characters from src position to dst position
  FXString& move(FXint dst,FXint src,FXint n);

  /// Remove one character
  FXString& erase(FXint pos);

  /// Remove substring
  FXString& erase(FXint pos,FXint n);

  /// Return number of occurrences of ch in string
  FXint contains(FXchar ch) const throw();

  /// Return number of occurrences of string sub in string
  FXint contains(const FXchar* sub,FXint n) const throw();

  /// Return number of occurrences of string sub in string
  FXint contains(const FXchar* sub) const throw();

  /// Return number of occurrences of string sub in string
  FXint contains(const FXString& sub) const throw();

  /// Substitute one character by another
  FXString& substitute(FXchar org,FXchar sub,bool all=true) throw();

  /// Substitute one string by another
  FXString& substitute(const FXchar* org,FXint olen,const FXchar *rep,FXint rlen,bool all=true);

  /// Substitute one string by another
  FXString& substitute(const FXchar* org,const FXchar *rep,bool all=true);

  /// Substitute one string by another
  FXString& substitute(const FXString& org,const FXString& rep,bool all=true);

  /// Simplify whitespace in string
  FXString& simplify();

  /// Remove leading and trailing whitespace
  FXString& trim();

  /// Remove leading whitespace
  FXString& trimBegin();

  /// Remove trailing whitespace
  FXString& trimEnd();

  /// Truncate string at pos
  FXString& trunc(FXint pos);

  /// Clear
  FXString& clear();

  /// Get left most part
  FXString left(FXint n) const;

  /// Get right most part
  FXString right(FXint n) const;

  /// Get some part in the middle
  FXString mid(FXint pos,FXint n=1073741824) const;

  /**
  * Return all characters before the n-th occurrence of ch,
  * searching from the beginning of the string. If the character
  * is not found, return the entire string.  If n<=0, return
  * the empty string.
  */
  FXString before(FXchar ch,FXint n=1) const;

  /**
  * Return all characters before the n-th occurrence of ch,
  * searching from the end of the string. If the character
  * is not found, return the empty string. If n<=0, return
  * the entire string.
  */
  FXString rbefore(FXchar ch,FXint n=1) const;

  /**
  * Return all characters after the nth occurrence of ch,
  * searching from the beginning of the string. If the character
  * is not found, return the empty string.  If n<=0, return
  * the entire string.
  */
  FXString after(FXchar ch,FXint n=1) const;

  /**
  * Return all characters after the nth occurrence of ch,
  * searching from the end of the string. If the character
  * is not found, return the entire string. If n<=0, return
  * the empty string.
  */
  FXString rafter(FXchar ch,FXint n=1) const;

  /// Find a character, searching forward; return position or -1
  FXint find(FXchar c,FXint pos=0) const throw();

  /// Find a character, searching backward; return position or -1
  FXint rfind(FXchar c,FXint pos=2147483647) const throw();

  /// Find n-th occurrence of character, searching forward; return position or -1
  FXint find(FXchar c,FXint pos,FXint n) const throw();

  /// Find n-th occurrence of character, searching backward; return position or -1
  FXint rfind(FXchar c,FXint pos,FXint n) const throw();

  /// Find a substring of length n, searching forward; return position or -1
  FXint find(const FXchar* substr,FXint n,FXint pos) const throw();

  /// Find a substring of length n, searching backward; return position or -1
  FXint rfind(const FXchar* substr,FXint n,FXint pos) const throw();

  /// Find a substring, searching forward; return position or -1
  FXint find(const FXchar* substr,FXint pos=0) const throw();

  /// Find a substring, searching backward; return position or -1
  FXint rfind(const FXchar* substr,FXint pos=2147483647) const throw();

  /// Find a substring, searching forward; return position or -1
  FXint find(const FXString& substr,FXint pos=0) const throw();

  /// Find a substring, searching backward; return position or -1
  FXint rfind(const FXString& substr,FXint pos=2147483647) const throw();

  /// Find first character in the set of size n, starting from pos; return position or -1
  FXint find_first_of(const FXchar* set,FXint n,FXint pos) const throw();

  /// Find first character in the set, starting from pos; return position or -1
  FXint find_first_of(const FXchar* set,FXint pos=0) const throw();

  /// Find first character in the set, starting from pos; return position or -1
  FXint find_first_of(const FXString& set,FXint pos=0) const throw();

  /// Find first character, starting from pos; return position or -1
  FXint find_first_of(FXchar c,FXint pos=0) const throw();

  /// Find last character in the set of size n, starting from pos; return position or -1
  FXint find_last_of(const FXchar* set,FXint n,FXint pos) const throw();

  /// Find last character in the set, starting from pos; return position or -1
  FXint find_last_of(const FXchar* set,FXint pos=2147483647) const throw();

  /// Find last character in the set, starting from pos; return position or -1
  FXint find_last_of(const FXString& set,FXint pos=2147483647) const throw();

  /// Find last character, starting from pos; return position or -1
  FXint find_last_of(FXchar c,FXint pos=0) const throw();

  /// Find first character NOT in the set of size n, starting from pos; return position or -1
  FXint find_first_not_of(const FXchar* set,FXint n,FXint pos) const throw();

  /// Find first character NOT in the set, starting from pos; return position or -1
  FXint find_first_not_of(const FXchar* set,FXint pos=0) const throw();

  /// Find first character NOT in the set, starting from pos; return position or -1
  FXint find_first_not_of(const FXString& set,FXint pos=0) const throw();

  /// Find first character NOT equal to c, starting from pos; return position or -1
  FXint find_first_not_of(FXchar c,FXint pos=0) const throw();

  /// Find last character NOT in the set of size n, starting from pos; return position or -1
  FXint find_last_not_of(const FXchar* set,FXint n,FXint pos) const throw();

  /// Find last character NOT in the set, starting from pos; return position or -1
  FXint find_last_not_of(const FXchar* set,FXint pos=2147483647) const throw();

  /// Find last character NOT in the set, starting from pos; return position or -1
  FXint find_last_not_of(const FXString& set,FXint pos=2147483647) const throw();

  /// Find last character NOT equal to c, starting from pos; return position or -1
  FXint find_last_not_of(FXchar c,FXint pos=0) const throw();

  /// Format a string a-la printf
  FXString& format(const FXchar* fmt,...) FX_PRINTF(2,3) ;
  FXString& vformat(const FXchar* fmt,va_list args);

  /// Scan a string a-la scanf
  FXint scan(const FXchar* fmt,...) const FX_SCANF(2,3) ;
  FXint vscan(const FXchar* fmt,va_list args) const;

  /// Get hash value
  FXuint hash() const throw();

  /// Truncates the string to the specified length
  void truncate(FXint len) { length(len); }

  /// Inserts an argument into the lowest numbered %x. Specifying a negative number
  /// for \em fieldwidth fills with zeros instead of spaces for numbers and for text
  /// causes alignment to the left instead of right
  FXString &arg(const FXString &str, FXint fieldwidth=0);
  FXString &arg(const char *str, FXint fieldwidth=0) { return arg(FXString(str), fieldwidth); }
  FXString &arg(const FXwchar *str, FXint fieldwidth=0) { return arg(FXString(str), fieldwidth); }
  FXString &arg(const FXnchar *str, FXint fieldwidth=0) { return arg(FXString(str), fieldwidth); }
  FXString &arg(char c, FXint fieldwidth=0);
  FXString &arg(FXlong num,   FXint fieldwidth=0, FXint base=10);
  FXString &arg(FXulong num,  FXint fieldwidth=0, FXint base=10);
  FXString &arg(FXint num,    FXint fieldwidth=0, FXint base=10) { return (base!=10) ? arg((FXulong)(FXuint) num, fieldwidth, base) : arg((FXlong) num, fieldwidth, base); }
  FXString &arg(FXuint num,   FXint fieldwidth=0, FXint base=10) { return arg((FXulong) num, fieldwidth, base); }
  FXString &arg(FXshort num,  FXint fieldwidth=0, FXint base=10) { return (base!=10) ? arg((FXulong)(FXushort) num, fieldwidth, base) : arg((FXlong) num, fieldwidth, base); }
  FXString &arg(FXushort num, FXint fieldwidth=0, FXint base=10) { return arg((FXulong) num, fieldwidth, base); }
#if !(defined(__LP64__) || defined(_LP64) || (_MIPS_SZLONG == 64) || (__WORDSIZE == 64))
  // Must declare overloads for long when long!=FXlong
  FXString &arg(long num,     FXint fieldwidth=0, FXint base=10) { return arg((base!=10) ? (FXulong)((unsigned long) num) : (FXlong) num, fieldwidth, base); }
  FXString &arg(unsigned long num, FXint fieldwidth=0, FXint base=10) { return arg((FXulong) num, fieldwidth, base); }
#endif
  FXString &arg(double num,   FXint fieldwidth=0, FXchar fmt='g', int prec=-1);
  FXString &arg(void *ptr,    FXint fieldwidth=-FXint(sizeof(FXuval)*2)) { return arg((FXulong)(FXuval) ptr, fieldwidth, 16); }

  /// Generates statically a textual representation of a number
  static FXString number(FXlong num,   FXint base=10);
  static FXString number(FXulong num,  FXint base=10);
  static FXString number(FXint num,    FXint base=10) { return (base!=10) ? number((FXulong)(FXuint) num, base) : number((FXlong) num, base); }
  static FXString number(FXuint num,   FXint base=10) { return number((FXulong) num, base); }
  static FXString number(FXshort num,  FXint base=10) { return (base!=10) ? number((FXulong)(FXushort) num, base) : number((FXlong) num, base); }
  static FXString number(FXushort num, FXint base=10) { return number((FXulong) num, base); }
  static FXString number(double num, FXchar fmt='g', int prec=-1);
  static FXString number(void *ptr) { return number((FXulong)(FXuval) ptr, 16); }

  /// Converts a string to a number
  FXlong toLong(bool *ok=0, FXint base=10) const throw();
  FXulong toULong(bool *ok=0, FXint base=10) const throw();
  FXint toInt(bool *ok=0, FXint base=10) const throw() { return (FXint) toLong(ok, base); }
  FXuint toUInt(bool *ok=0, FXint base=10) const throw() { return (FXuint) toULong(ok, base); }
  double toDouble(bool *ok=0) const throw();

  /// Returns a null string
  static const FXString &nullStr() throw();

private:
  inline FXDLLLOCAL void getLowestInsert(FXint &pos, FXint &len);
  inline FXDLLLOCAL void resetInserts();
  inline FXDLLLOCAL void shiftInserts(FXint pos, FXint diff);
  inline FXDLLLOCAL void doneInsert();
  FXDLLLOCAL void calcInserts();
  FXDLLLOCAL FXString numToText(FXulong num, FXint fw, FXint base);
  FXDLLLOCAL FXulong textToNum(const FXchar *str, bool *ok, FXint base) const throw();
public:

  /// Compare
  friend FXAPI FXint compare(const FXchar* s1,const FXchar* s2) throw();
  friend FXAPI FXint compare(const FXchar* s1,const FXString& s2) throw();
  friend FXAPI FXint compare(const FXString& s1,const FXchar* s2) throw();
  friend FXAPI FXint compare(const FXString& s1,const FXString& s2) throw();

  /// Compare up to n
  friend FXAPI FXint compare(const FXchar* s1,const FXchar* s2,FXint n) throw();
  friend FXAPI FXint compare(const FXchar* s1,const FXString& s2,FXint n) throw();
  friend FXAPI FXint compare(const FXString& s1,const FXchar* s2,FXint n) throw();
  friend FXAPI FXint compare(const FXString& s1,const FXString& s2,FXint n) throw();

  /// Compare case insensitive
  friend FXAPI FXint comparecase(const FXchar* s1,const FXchar* s2) throw();
  friend FXAPI FXint comparecase(const FXchar* s1,const FXString& s2) throw();
  friend FXAPI FXint comparecase(const FXString& s1,const FXchar* s2) throw();
  friend FXAPI FXint comparecase(const FXString& s1,const FXString& s2) throw();

  /// Compare case insensitive up to n
  friend FXAPI FXint comparecase(const FXchar* s1,const FXchar* s2,FXint n) throw();
  friend FXAPI FXint comparecase(const FXchar* s1,const FXString& s2,FXint n) throw();
  friend FXAPI FXint comparecase(const FXString& s1,const FXchar* s2,FXint n) throw();
  friend FXAPI FXint comparecase(const FXString& s1,const FXString& s2,FXint n) throw();

  /// Compare with numeric interpretation
  friend FXAPI FXint compareversion(const FXchar* s1,const FXchar* s2) throw();
  friend FXAPI FXint compareversion(const FXchar* s1,const FXString& s2) throw();
  friend FXAPI FXint compareversion(const FXString& s1,const FXchar* s2) throw();
  friend FXAPI FXint compareversion(const FXString& s1,const FXString& s2) throw();

  /// Comparison operators
  friend FXAPI bool operator==(const FXString& s1,const FXString& s2) throw();
  friend FXAPI bool operator==(const FXString& s1,const FXchar* s2) throw();
  friend FXAPI bool operator==(const FXchar* s1,const FXString& s2) throw();

  friend FXAPI bool operator!=(const FXString& s1,const FXString& s2) throw();
  friend FXAPI bool operator!=(const FXString& s1,const FXchar* s2) throw();
  friend FXAPI bool operator!=(const FXchar* s1,const FXString& s2) throw();

  friend FXAPI bool operator<(const FXString& s1,const FXString& s2) throw();
  friend FXAPI bool operator<(const FXString& s1,const FXchar* s2) throw();
  friend FXAPI bool operator<(const FXchar* s1,const FXString& s2) throw();

  friend FXAPI bool operator<=(const FXString& s1,const FXString& s2) throw();
  friend FXAPI bool operator<=(const FXString& s1,const FXchar* s2) throw();
  friend FXAPI bool operator<=(const FXchar* s1,const FXString& s2) throw();

  friend FXAPI bool operator>(const FXString& s1,const FXString& s2) throw();
  friend FXAPI bool operator>(const FXString& s1,const FXchar* s2) throw();
  friend FXAPI bool operator>(const FXchar* s1,const FXString& s2) throw();

  friend FXAPI bool operator>=(const FXString& s1,const FXString& s2) throw();
  friend FXAPI bool operator>=(const FXString& s1,const FXchar* s2) throw();
  friend FXAPI bool operator>=(const FXchar* s1,const FXString& s2) throw();

  /// Append operators
  FXString& operator+=(const FXString& s);
  FXString& operator+=(const FXchar* s);
  FXString& operator+=(const FXwchar* s);
  FXString& operator+=(const FXnchar* s);
  FXString& operator+=(FXchar c);

  /// Concatenate one FXString with another
  friend FXAPI FXString operator+(const FXString& s1,const FXString& s2);

  /// Concatenate FXString and a string
  friend FXAPI FXString operator+(const FXString& s1,const FXchar* s2);
  friend FXAPI FXString operator+(const FXString& s1,const FXwchar* s2);
  friend FXAPI FXString operator+(const FXString& s1,const FXnchar* s2);

  /// Concatenate string and FXString
  friend FXAPI FXString operator+(const FXchar* s1,const FXString& s2);
  friend FXAPI FXString operator+(const FXwchar* s1,const FXString& s2);
  friend FXAPI FXString operator+(const FXnchar* s1,const FXString& s2);

  /// Concatenate string and single character
  friend FXAPI FXString operator+(const FXString& s,FXchar c);
  friend FXAPI FXString operator+(FXchar c,const FXString& s);

  /// Saving to a stream
  friend inline FXStream& operator<<(FXStream& store,const FXString& s);

  /// Load from a stream
  friend inline FXStream& operator>>(FXStream& store,FXString& s);

  /// Format a string a-la printf
  friend FXAPI FXString FXStringFormat(const FXchar* fmt,...) FX_PRINTF(1,2) ;
  friend FXAPI FXString FXStringVFormat(const FXchar* fmt,va_list args);

  /**
  * Convert integer number to a string, using the given number
  * base, which must be between 2 and 16.
  */
  friend FXAPI FXString FXStringVal(FXint num,FXint base);
  friend FXAPI FXString FXStringVal(FXuint num,FXint base);

  /**
  * Convert long integer number to a string, using the given number
  * base, which must be between 2 and 16.
  */
  friend FXAPI FXString FXStringVal(FXlong num,FXint base);
  friend FXAPI FXString FXStringVal(FXulong num,FXint base);

  /**
  * Convert real number to a string, using the given procision and
  * exponential notation mode, which may be FALSE (never), TRUE (always), or
  * MAYBE (when needed).
  */
  friend FXAPI FXString FXStringVal(FXfloat num,FXint prec,FXint exp);
  friend FXAPI FXString FXStringVal(FXdouble num,FXint prec,FXint exp);

  /// Convert string to a integer number, assuming given number base
  friend FXAPI FXint FXIntVal(const FXString& s,FXint base);
  friend FXAPI FXuint FXUIntVal(const FXString& s,FXint base);

  /// Convert string to long integer number, assuming given number base
  friend FXAPI FXlong FXLongVal(const FXString& s,FXint base);
  friend FXAPI FXulong FXULongVal(const FXString& s,FXint base);

  /// Convert string into real number
  friend FXAPI FXfloat FXFloatVal(const FXString& s);
  friend FXAPI FXdouble FXDoubleVal(const FXString& s);

  /// Return utf8 from ascii containing unicode escapes
  friend FXAPI FXString fromAscii(const FXString& s);

  /// Return ascii containing unicode escapes from utf8
  friend FXAPI FXString toAscii(const FXString& s);

  /// Escape special characters in a string
  friend FXAPI FXString escape(const FXString& s);

  /// Unescape special characters in a string
  friend FXAPI FXString unescape(const FXString& s);

  /// Return normalized string, i.e. reordering of diacritical marks
  friend FXAPI FXString normalize(const FXString& s);

  /// Return normalized decomposition of string
  friend FXAPI FXString decompose(const FXString& s,FXuint kind);

  /// Return normalized composition of string; this first performs normalized decomposition
  friend FXAPI FXString compose(const FXString& s,FXuint kind);

  /// Swap two strings
  friend inline void swap(FXString& a,FXString& b);

  /// Convert to and from dos
  friend FXAPI FXString& unixToDos(FXString& str);
  friend FXAPI FXString& dosToUnix(FXString& str);

  /// Delete
 ~FXString();
  };


inline void swap(FXString& a,FXString& b){ FXchar *t=a.str; a.str=b.str; b.str=t; }

extern FXAPI FXint compare(const FXchar* s1,const FXchar* s2) throw();
extern FXAPI FXint compare(const FXchar* s1,const FXString& s2) throw();
extern FXAPI FXint compare(const FXString& s1,const FXchar* s2) throw();
extern FXAPI FXint compare(const FXString& s1,const FXString& s2) throw();

extern FXAPI FXint compare(const FXchar* s1,const FXchar* s2,FXint n) throw();
extern FXAPI FXint compare(const FXchar* s1,const FXString& s2,FXint n) throw();
extern FXAPI FXint compare(const FXString& s1,const FXchar* s2,FXint n) throw();
extern FXAPI FXint compare(const FXString& s1,const FXString& s2,FXint n) throw();

extern FXAPI FXint comparecase(const FXchar* s1,const FXchar* s2) throw();
extern FXAPI FXint comparecase(const FXchar* s1,const FXString& s2) throw();
extern FXAPI FXint comparecase(const FXString& s1,const FXchar* s2) throw();
extern FXAPI FXint comparecase(const FXString& s1,const FXString& s2) throw();

extern FXAPI FXint comparecase(const FXchar* s1,const FXchar* s2,FXint n) throw();
extern FXAPI FXint comparecase(const FXchar* s1,const FXString& s2,FXint n) throw();
extern FXAPI FXint comparecase(const FXString& s1,const FXchar* s2,FXint n) throw();
extern FXAPI FXint comparecase(const FXString& s1,const FXString& s2,FXint n) throw();

extern FXAPI FXint compareversion(const FXchar* s1,const FXchar* s2) throw();
extern FXAPI FXint compareversion(const FXchar* s1,const FXString& s2) throw();
extern FXAPI FXint compareversion(const FXString& s1,const FXchar* s2) throw();
extern FXAPI FXint compareversion(const FXString& s1,const FXString& s2) throw();

extern FXAPI bool operator==(const FXString& s1,const FXString& s2) throw();
extern FXAPI bool operator==(const FXString& s1,const FXchar* s2) throw();
extern FXAPI bool operator==(const FXchar* s1,const FXString& s2) throw();

extern FXAPI bool operator!=(const FXString& s1,const FXString& s2) throw();
extern FXAPI bool operator!=(const FXString& s1,const FXchar* s2) throw();
extern FXAPI bool operator!=(const FXchar* s1,const FXString& s2) throw();

extern FXAPI bool operator<(const FXString& s1,const FXString& s2) throw();
extern FXAPI bool operator<(const FXString& s1,const FXchar* s2) throw();
extern FXAPI bool operator<(const FXchar* s1,const FXString& s2) throw();

extern FXAPI bool operator<=(const FXString& s1,const FXString& s2) throw();
extern FXAPI bool operator<=(const FXString& s1,const FXchar* s2) throw();
extern FXAPI bool operator<=(const FXchar* s1,const FXString& s2) throw();

extern FXAPI bool operator>(const FXString& s1,const FXString& s2) throw();
extern FXAPI bool operator>(const FXString& s1,const FXchar* s2) throw();
extern FXAPI bool operator>(const FXchar* s1,const FXString& s2) throw();

extern FXAPI bool operator>=(const FXString& s1,const FXString& s2) throw();
extern FXAPI bool operator>=(const FXString& s1,const FXchar* s2) throw();
extern FXAPI bool operator>=(const FXchar* s1,const FXString& s2) throw();

extern FXAPI FXString operator+(const FXString& s1,const FXString& s2);

extern FXAPI FXString operator+(const FXString& s1,const FXchar* s2);
extern FXAPI FXString operator+(const FXString& s1,const FXwchar* s2);
extern FXAPI FXString operator+(const FXString& s1,const FXnchar* s2);

extern FXAPI FXString operator+(const FXchar* s1,const FXString& s2);
extern FXAPI FXString operator+(const FXwchar* s1,const FXString& s2);
extern FXAPI FXString operator+(const FXnchar* s1,const FXString& s2);

extern FXAPI FXString operator+(const FXString& s,FXchar c);
extern FXAPI FXString operator+(FXchar c,const FXString& s);

extern FXAPI FXString FXStringFormat(const FXchar* fmt,...) FX_PRINTF(1,2) ;
extern FXAPI FXString FXStringVFormat(const FXchar* fmt,va_list args);

extern FXAPI FXString FXStringVal(FXint num,FXint base=10);
extern FXAPI FXString FXStringVal(FXuint num,FXint base=10);
extern FXAPI FXString FXStringVal(FXlong num,FXint base=10);
extern FXAPI FXString FXStringVal(FXulong num,FXint base=10);
extern FXAPI FXString FXStringVal(FXfloat num,FXint prec=6,FXint exp=MAYBE);
extern FXAPI FXString FXStringVal(FXdouble num,FXint prec=6,FXint exp=MAYBE);

extern FXAPI FXint FXIntVal(const FXString& s,FXint base=10);
extern FXAPI FXuint FXUIntVal(const FXString& s,FXint base=10);
extern FXAPI FXlong FXLongVal(const FXString& s,FXint base=10);
extern FXAPI FXulong FXULongVal(const FXString& s,FXint base=10);
extern FXAPI FXfloat FXFloatVal(const FXString& s);
extern FXAPI FXdouble FXDoubleVal(const FXString& s);

extern FXAPI FXString fromAscii(const FXString& s);
extern FXAPI FXString toAscii(const FXString& s);

extern FXAPI FXString escape(const FXString& s);
extern FXAPI FXString unescape(const FXString& s);

extern FXAPI FXString normalize(const FXString& s);
extern FXAPI FXString decompose(const FXString& s,FXuint kind);
extern FXAPI FXString compose(const FXString& s,FXuint kind);

extern FXAPI FXString& unixToDos(FXString& str);
extern FXAPI FXString& dosToUnix(FXString& str);

//! For Qt emulation
typedef FXString QString;

}

#endif
