/********************************************************************************
*                                                                               *
*                    W i d e   S t r i n g   O b j e c t                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXWString.h,v 1.11 2004/08/28 01:16:10 fox Exp $                         *
********************************************************************************/
#ifndef FXWSTRING_H
#define FXWSTRING_H

#include "fxdefs.h"

namespace FX {



//////////////////////////////  UNDER DEVELOPMENT  //////////////////////////////


/**
* FXWString provides a "wide" string class suitable for storing Unicode strings.
*/
class FXAPI FXWString {
private:
  FXwchar* str;
public:
  static const FXwchar null[];
  static const FXwchar hex[17];
  static const FXwchar HEX[17];
public:

  /// Create empty string
  FXWString();

  /// Copy construct
  FXWString(const FXWString& s);

  /// Construct and init
  FXWString(const FXwchar* s);

  /// Construct and init with substring
  FXWString(const FXwchar* s,FXint n);

  /// Construct and fill with constant
  FXWString(FXwchar c,FXint n);

  /// Construct string from two parts
  FXWString(const FXwchar *s1,const FXwchar* s2);

  /// Change the length of the string to len
  void length(FXint len);

  /// Length of text
  FXint length() const { return ((FXint*)str)[-1]; }

  /// Get text contents
  const FXwchar* text() const { return (const FXwchar*)str; }

  /// See if string is empty
  FXbool empty() const { return (((FXint*)str)[-1]==0); }

  /// Return a non-const reference to the ith character
  FXwchar& operator[](FXint i){ return str[i]; }

  /// Return a const reference to the ith character
  const FXwchar& operator[](FXint i) const { return str[i]; }

  /// Assign another string to this
  FXWString& operator=(const FXWString& s);

  /// Assign a C-style string to this
  FXWString& operator=(const FXwchar* s);

  /// Fill with a constant
  FXWString& fill(FXwchar c,FXint n);

  /// Fill up to current length
  FXWString& fill(FXwchar c);

  /// Convert to lower case
  FXWString& lower();

  /// Convert to upper case
  FXWString& upper();

  /// Return num partition(s) of string separated by delimiter delim
  FXWString section(FXwchar delim,FXint start,FXint num=1) const;

  /// Return num partition(s) of string separated by delimiters in delim
  FXWString section(const FXwchar* delim,FXint n,FXint start,FXint num=1) const;

  /// Return num partition(s) of string separated by delimiters in delim
  FXWString section(const FXwchar* delim,FXint start,FXint num=1) const;

  /// Return num partition(s) of string separated by delimiters in delim
  FXWString section(const FXWString& delim,FXint start,FXint num=1) const;

  /// Assign character c to this string
  FXWString& assign(FXwchar c);

  /// Assign n characters c to this string
  FXWString& assign(FXwchar c,FXint n);

  /// Assign first n characters of string s to this string
  FXWString& assign(const FXwchar *s,FXint n);

  /// Assign string s to this string
  FXWString& assign(const FXWString& s);

  /// Assign string s to this string
  FXWString& assign(const FXwchar *s);

  /// Insert character at specified position
  FXWString& insert(FXint pos,FXwchar c);

  /// Insert n characters c at specified position
  FXWString& insert(FXint pos,FXwchar c,FXint n);

  /// Insert first n characters of string at specified position
  FXWString& insert(FXint pos,const FXwchar* s,FXint n);

  /// Insert string at specified position
  FXWString& insert(FXint pos,const FXWString& s);

  /// Insert string at specified position
  FXWString& insert(FXint pos,const FXwchar* s);

  /// Prepend string with input character
  FXWString& prepend(FXwchar c);

  /// Prepend string with n characters c
  FXWString& prepend(FXwchar c,FXint n);

  /// Prepend string with first n characters of input string
  FXWString& prepend(const FXwchar *s,FXint n);

  /// Prepend string with input string
  FXWString& prepend(const FXWString& s);

  /// Prepend string with input string
  FXWString& prepend(const FXwchar *s);

  /// Append input character to this string
  FXWString& append(FXwchar c);

  /// Append input n characters c to this string
  FXWString& append(FXwchar c,FXint n);

  /// Append first n characters of input string to this string
  FXWString& append(const FXwchar *s,FXint n);

  /// Append input string to this string
  FXWString& append(const FXWString& s);

  /// Append input string to this string
  FXWString& append(const FXwchar *s);

  /// Replace a single character
  FXWString& replace(FXint pos,FXwchar c);

  /// Replace the m characters at pos with n characters c
  FXWString& replace(FXint pos,FXint m,FXwchar c,FXint n);

  /// Replaces the m characters at pos with first n characters of input string
  FXWString& replace(FXint pos,FXint m,const FXwchar *s,FXint n);

  /// Replace the m characters at pos with input string
  FXWString& replace(FXint pos,FXint m,const FXWString& s);

  /// Replace the m characters at pos with input string
  FXWString& replace(FXint pos,FXint m,const FXwchar *s);

  /// Remove substring
  FXWString& remove(FXint pos,FXint n=1);

  /// Return number of occurrences of ch in string
  FXint contains(FXwchar ch);

  /// Return number of occurrences of string sub in string
  FXint contains(const FXwchar* sub,FXint n);

  /// Return number of occurrences of string sub in string
  FXint contains(const FXwchar* sub);

  /// Return number of occurrences of string sub in string
  FXint contains(const FXWString& sub);

  /// Substitute one character by another
  FXWString& substitute(FXwchar org,FXwchar sub,FXbool all=TRUE);

  /// Substitute one string by another
  FXWString& substitute(const FXwchar* org,FXint olen,const FXwchar *rep,FXint rlen,FXbool all=TRUE);

  /// Substitute one string by another
  FXWString& substitute(const FXwchar* org,const FXwchar *rep,FXbool all=TRUE);

  /// Substitute one string by another
  FXWString& substitute(const FXWString& org,const FXWString& rep,FXbool all=TRUE);

  /// Simplify whitespace in string
  FXWString& simplify();

  /// Remove leading and trailing whitespace
  FXWString& trim();

  /// Remove leading whitespace
  FXWString& trimBegin();

  /// Remove trailing whitespace
  FXWString& trimEnd();

  /// Truncate string at pos
  FXWString& trunc(FXint pos);

  /// Clear
  FXWString& clear();

  /// Get leftmost part
  FXWString left(FXint n) const;

  /// Get rightmost part
  FXWString right(FXint n) const;

  /// Get some part in the middle
  FXWString mid(FXint pos,FXint n) const;

  /**
  * Return all characters before the n-th occurrence of ch,
  * searching from the beginning of the string. If the character
  * is not found, return the entire string.  If n<=0, return
  * the empty string.
  */
  FXWString before(FXwchar ch,FXint n=1) const;

  /**
  * Return all characters before the n-th occurrence of ch,
  * searching from the end of the string. If the character
  * is not found, return the empty string. If n<=0, return
  * the entire string.
  */
  FXWString rbefore(FXwchar ch,FXint n=1) const;

  /**
  * Return all characters after the nth occurrence of ch,
  * searching from the beginning of the string. If the character
  * is not found, return the empty string.  If n<=0, return
  * the entire string.
  */
  FXWString after(FXwchar ch,FXint n=1) const;

  /**
  * Return all characters after the nth occurrence of ch,
  * searching from the end of the string. If the character
  * is not found, return the entire string. If n<=0, return
  * the empty string.
  */
  FXWString rafter(FXwchar ch,FXint n=1) const;

  /// Find a character, searching forward; return position or -1
  FXint find(FXwchar c,FXint pos=0) const;

  /// Find a character, searching backward; return position or -1
  FXint rfind(FXwchar c,FXint pos=2147483647) const;

  // Find n-th occurrence of character, searching forward; return position or -1
  FXint find(FXwchar c,FXint pos,FXint n) const;

  // Find n-th occurrence of character, searching backward; return position or -1
  FXint rfind(FXwchar c,FXint pos,FXint n) const;

  /// Find a substring of length n, searching forward; return position or -1
  FXint find(const FXwchar* substr,FXint n,FXint pos) const;

  /// Find a substring of length n, searching backward; return position or -1
  FXint rfind(const FXwchar* substr,FXint n,FXint pos) const;

  /// Find a substring, searching forward; return position or -1
  FXint find(const FXwchar* substr,FXint pos=0) const;

  /// Find a substring, searching backward; return position or -1
  FXint rfind(const FXwchar* substr,FXint pos=2147483647) const;

  /// Find a substring, searching forward; return position or -1
  FXint find(const FXWString &substr,FXint pos=0) const;

  /// Find a substring, searching backward; return position or -1
  FXint rfind(const FXWString &substr,FXint pos=2147483647) const;

  /// Find first character in the set of size n, starting from pos; return position or -1
  FXint find_first_of(const FXwchar *set,FXint n,FXint pos) const;

  /// Find first character in the set, starting from pos; return position or -1
  FXint find_first_of(const FXwchar *set,FXint pos=0) const;

  /// Find first character in the set, starting from pos; return position or -1
  FXint find_first_of(const FXWString &set,FXint pos=0) const;

  /// Find first character, starting from pos; return position or -1
  FXint find_first_of(FXwchar c,FXint pos=0) const;

  /// Find last character in the set of size n, starting from pos; return position or -1
  FXint find_last_of(const FXwchar *set,FXint n,FXint pos) const;

  /// Find last character in the set, starting from pos; return position or -1
  FXint find_last_of(const FXwchar *set,FXint pos=2147483647) const;

  /// Find last character in the set, starting from pos; return position or -1
  FXint find_last_of(const FXWString &set,FXint pos=2147483647) const;

  /// Find last character, starting from pos; return position or -1
  FXint find_last_of(FXwchar c,FXint pos=0) const;

  /// Find first character NOT in the set of size n, starting from pos; return position or -1
  FXint find_first_not_of(const FXwchar *set,FXint n,FXint pos) const;

  /// Find first character NOT in the set, starting from pos; return position or -1
  FXint find_first_not_of(const FXwchar *set,FXint pos=0) const;

  /// Find first character NOT in the set, starting from pos; return position or -1
  FXint find_first_not_of(const FXWString &set,FXint pos=0) const;

  /// Find first character NOT equal to c, starting from pos; return position or -1
  FXint find_first_not_of(FXwchar c,FXint pos=0) const;

  /// Find last character NOT in the set of size n, starting from pos; return position or -1
  FXint find_last_not_of(const FXwchar *set,FXint n,FXint pos) const;

  /// Find last character NOT in the set, starting from pos; return position or -1
  FXint find_last_not_of(const FXwchar *set,FXint pos=2147483647) const;

  /// Find last character NOT in the set, starting from pos; return position or -1
  FXint find_last_not_of(const FXWString &set,FXint pos=2147483647) const;

  /// Find last character NOT equal to c, starting from pos; return position or -1
  FXint find_last_not_of(FXwchar c,FXint pos=0) const;

  /// Find number of occurrences of character in string
  FXint count(FXwchar c) const;

  /// Format a string a-la printf
  // FXWString& format(const char *fmt,...) FX_PRINTF(2,3) ;
  // FXWString& vformat(const char *fmt,va_list args);

  /// Scan a string a-la scanf
  // FXint scan(const char *fmt,...) const FX_SCANF(2,3) ;
  // FXint vscan(const char *fmt,va_list args) const;

  /// Get hash value
  FXuint hash() const;

  /// Compare
  friend FXAPI FXint compare(const FXwchar *s1,const FXwchar *s2);
  friend FXAPI FXint compare(const FXwchar *s1,const FXWString &s2);
  friend FXAPI FXint compare(const FXWString &s1,const FXwchar *s2);
  friend FXAPI FXint compare(const FXWString &s1,const FXWString &s2);

  /// Compare up to n
  friend FXAPI FXint compare(const FXwchar *s1,const FXwchar *s2,FXint n);
  friend FXAPI FXint compare(const FXwchar *s1,const FXWString &s2,FXint n);
  friend FXAPI FXint compare(const FXWString &s1,const FXwchar *s2,FXint n);
  friend FXAPI FXint compare(const FXWString &s1,const FXWString &s2,FXint n);

  /// Compare case insensitive
  friend FXAPI FXint comparecase(const FXwchar *s1,const FXwchar *s2);
  friend FXAPI FXint comparecase(const FXwchar *s1,const FXWString &s2);
  friend FXAPI FXint comparecase(const FXWString &s1,const FXwchar *s2);
  friend FXAPI FXint comparecase(const FXWString &s1,const FXWString &s2);

  /// Compare case insensitive up to n
  friend FXAPI FXint comparecase(const FXwchar *s1,const FXwchar *s2,FXint n);
  friend FXAPI FXint comparecase(const FXwchar *s1,const FXWString &s2,FXint n);
  friend FXAPI FXint comparecase(const FXWString &s1,const FXwchar *s2,FXint n);
  friend FXAPI FXint comparecase(const FXWString &s1,const FXWString &s2,FXint n);

  /// Comparison operators
  friend FXAPI FXbool operator==(const FXWString &s1,const FXWString &s2);
  friend FXAPI FXbool operator==(const FXWString &s1,const FXwchar *s2);
  friend FXAPI FXbool operator==(const FXwchar *s1,const FXWString &s2);

  friend FXAPI FXbool operator!=(const FXWString &s1,const FXWString &s2);
  friend FXAPI FXbool operator!=(const FXWString &s1,const FXwchar *s2);
  friend FXAPI FXbool operator!=(const FXwchar *s1,const FXWString &s2);

  friend FXAPI FXbool operator<(const FXWString &s1,const FXWString &s2);
  friend FXAPI FXbool operator<(const FXWString &s1,const FXwchar *s2);
  friend FXAPI FXbool operator<(const FXwchar *s1,const FXWString &s2);

  friend FXAPI FXbool operator<=(const FXWString &s1,const FXWString &s2);
  friend FXAPI FXbool operator<=(const FXWString &s1,const FXwchar *s2);
  friend FXAPI FXbool operator<=(const FXwchar *s1,const FXWString &s2);

  friend FXAPI FXbool operator>(const FXWString &s1,const FXWString &s2);
  friend FXAPI FXbool operator>(const FXWString &s1,const FXwchar *s2);
  friend FXAPI FXbool operator>(const FXwchar *s1,const FXWString &s2);

  friend FXAPI FXbool operator>=(const FXWString &s1,const FXWString &s2);
  friend FXAPI FXbool operator>=(const FXWString &s1,const FXwchar *s2);
  friend FXAPI FXbool operator>=(const FXwchar *s1,const FXWString &s2);

  /// Append operators
  FXWString& operator+=(const FXWString& s);
  FXWString& operator+=(const FXwchar* s);
  FXWString& operator+=(FXwchar c);

  /// Concatenate two strings
  friend FXAPI FXWString operator+(const FXWString& s1,const FXWString& s2);
  friend FXAPI FXWString operator+(const FXWString& s1,const FXwchar* s2);
  friend FXAPI FXWString operator+(const FXwchar* s1,const FXWString& s2);

  /// Concatenate with single character
  friend FXAPI FXWString operator+(const FXWString& s,FXwchar c);
  friend FXAPI FXWString operator+(FXwchar c,const FXWString& s);

  /// Saving to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXWString& s);

  /// Load from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXWString& s);

  /// Format a string a-la printf
  // friend FXAPI FXWString FXWStringFormat(const FXwchar *fmt,...) FX_PRINTF(1,2) ;
  // friend FXAPI FXWString FXWStringVFormat(const FXwchar *fmt,va_list args);

  /**
  * Convert integer number to a string, using the given number
  * base, which must be between 2 and 16.
  */
  // friend FXAPI FXWString FXWStringVal(FXint num,FXint base=10);
  // friend FXAPI FXWString FXWStringVal(FXuint num,FXint base=10);

  /**
  * Convert real number to a string, using the given procision and
  * exponential notation mode, which may be FALSE (never), TRUE (always), or
  * MAYBE (when needed).
  */
  // friend FXAPI FXWString FXWStringVal(FXfloat num,FXint prec=6,FXbool exp=MAYBE);
  // friend FXAPI FXWString FXWStringVal(FXdouble num,FXint prec=6,FXbool exp=MAYBE);

  /// Convert string to a integer number, assuming given number base
  // friend FXAPI FXint FXIntVal(const FXWString& s,FXint base=10);
  // friend FXAPI FXuint FXUIntVal(const FXWString& s,FXint base=10);

  /// Convert string into real number
  // friend FXAPI FXfloat FXFloatVal(const FXWString& s);
  // friend FXAPI FXdouble FXDoubleVal(const FXWString& s);

  /// Swap two strings
  friend FXAPI void swap(FXWString& a,FXWString& b){ FXwchar *t=a.str; a.str=b.str; b.str=t; }

  /// Delete
 ~FXWString();
  };

}

#endif

