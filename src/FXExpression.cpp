/********************************************************************************
*                                                                               *
*                      E x p r e s s i o n   E v a l u a t o r                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2006 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXExpression.cpp,v 1.5 2006/02/07 03:15:59 fox Exp $                     *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXExpression.h"


/*
  Notes:
  - Old as night, but recently rediscovered ;-)
  - Add common math functions.
  - We have bigger plans for the future...
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Tokens
enum Token {
  TOK_EOF,
  TOK_INT,
  TOK_INT_HEX,
  TOK_INT_BIN,
  TOK_INT_OCT,
  TOK_REAL,
  TOK_PLUS,
  TOK_MINUS,
  TOK_TIMES,
  TOK_DIVIDE,
  TOK_MODULO,
  TOK_LPAR,
  TOK_RPAR,
  TOK_LESS,
  TOK_GREATER,
  TOK_LESSEQ,
  TOK_GREATEREQ,
  TOK_EQUAL,
  TOK_NOTEQUAL,
  TOK_AND,
  TOK_OR,
  TOK_XOR,
  TOK_NOT,
  TOK_POWER,
  TOK_ASSIGN,
  TOK_RBRACKET,
  TOK_LBRACKET,
  TOK_SHIFTLEFT,
  TOK_SHIFTRIGHT,
  TOK_IDENT,
  TOK_PI,
  TOK_EULER,
  TOK_ERROR
  };


// Expression class
class Expression {
private:
  const FXchar *head;
  const FXchar *tail;
  Token         token;
public:
  Expression(const FXchar* txt):head(txt),tail(txt),token(TOK_EOF){}
  void gettok();
  bool element(FXdouble& result);
  bool primary(FXdouble& result);
  bool powexp(FXdouble& result);
  bool mulexp(FXdouble& result);
  bool addexp(FXdouble& result);
  bool compexp(FXdouble& result);
  bool expr(FXdouble& result);
  };


/*******************************************************************************/


// Obtain next token from input
void Expression::gettok(){
  register FXchar c;
  head=tail;
  while((c=*tail)!='\0'){
    switch(c){
      case ' ':
      case '\b':
      case '\t':
      case '\v':
      case '\f':
      case '\r':
      case '\n':
        head=++tail;
        break;
      case '=':
        token=TOK_ASSIGN; tail++;
        if(*tail=='='){ token=TOK_EQUAL; tail++; }
        return;
      case '<':
        token=TOK_LESS; tail++;
        if(*tail=='='){ token=TOK_LESSEQ; tail++; }
        else if(*tail=='<'){ token=TOK_SHIFTLEFT; tail++; }
        return;
      case '>':
        token=TOK_GREATER;
        tail++;
        if(*tail=='='){ token=TOK_GREATEREQ; tail++; }
        else if(*tail=='>'){ token=TOK_SHIFTRIGHT; tail++; }
        return;
      case '|':
        token=TOK_OR; tail++;
        return;
      case '&':
        token=TOK_AND; tail++;
        return;
      case '^':
        token=TOK_XOR; tail++;
        return;
      case '-':
        token=TOK_MINUS; tail++;
        return;
      case '+':
        token=TOK_PLUS; tail++;
        return;
      case '*':
        token=TOK_TIMES; tail++;
        if(*tail=='*'){ token=TOK_POWER; tail++; }
        return;
      case '/':
        token=TOK_DIVIDE; tail++;
        return;
      case '%':
        token=TOK_MODULO; tail++;
        return;
      case '!':
        token=TOK_NOT; tail++;
        if(*tail=='='){ token=TOK_NOTEQUAL; tail++; }
        return;
      case '(':
        token=TOK_LPAR; tail++;
        return;
      case ')':
        token=TOK_RPAR; tail++;
        return;
      case '.':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        token=TOK_INT;
        if(c=='0'){
          tail++;
          if(*tail=='x' || *tail=='X'){
            tail++;
            if(!Ascii::isHexDigit(*tail)){ token=TOK_ERROR; return; }
            tail++;
            while(Ascii::isHexDigit(*tail)) tail++;
            token=TOK_INT_HEX;
            return;
            }
          if(*tail=='b' || *tail=='B'){
            tail++;
            if(*tail!='0' && *tail!='1'){ token=TOK_ERROR; return; }
            tail++;
            while(*tail=='0' || *tail=='1') tail++;
            token=TOK_INT_BIN;
            return;
            }
          if('0'<=*tail && *tail<='7'){
            tail++;
            while('0'<=*tail && *tail<='7') tail++;
            if('7'<=*tail && *tail<='9'){
              token=TOK_ERROR;
              return;
              }
            token=TOK_INT_OCT;
            return;
            }
          }
        while(Ascii::isDigit(*tail)) tail++;
        if(*tail=='.'){
          token=TOK_REAL;
          tail++;
          while(Ascii::isDigit(*tail)) tail++;
          }
        if(*tail=='e' || *tail=='E'){
          token=TOK_REAL;
          tail++;
          if(*tail=='-' || *tail=='+') tail++;
          if(!Ascii::isDigit(*tail)){ token=TOK_ERROR; return; }
          tail++;
          while(Ascii::isDigit(*tail)) tail++;
          }
        return;
      case 'e':
        if(Ascii::isAlphaNumeric(tail[1])) goto ident;
        token=TOK_EULER;
        tail+=1;
        return;
      case 'p':
        if(tail[1]!='i') goto ident;
        if(Ascii::isAlphaNumeric(tail[2])) goto ident;
        token=TOK_PI;
        tail+=2;
        return;
      default:
ident:  token=TOK_ERROR;
        if(Ascii::isLetter(*tail)){
          token=TOK_IDENT;
          tail++;
          while(Ascii::isAlphaNumeric(*tail)) tail++;
          }
        return;
      }
    }
  token=TOK_EOF;
  }


// Element
bool Expression::element(FXdouble& result){
  switch(token){
    case TOK_LPAR:
      gettok();
      if(!expr(result)) return false;
      if(token!=TOK_RPAR) return false;
      break;
    case TOK_INT_HEX:
      result=(FXdouble)strtol(head+2,NULL,16);
      break;
    case TOK_INT_BIN:
      result=(FXdouble)strtol(head+2,NULL,2);
      break;
    case TOK_INT_OCT:
      result=(FXdouble)strtol(head+1,NULL,8);
      break;
    case TOK_INT:
      result=(FXdouble)strtol(head,NULL,10);
      break;
    case TOK_REAL:
      result=strtod(head,NULL);
      break;
    case TOK_EULER:
      result=EULER;
      break;
    case TOK_PI:
      result=PI;
      break;
/*    case TOK_IDENT:
    if(head[0]=='p' && head[1]=='i' && head+2==tail){
      result=PI;
      gettok();
      return true;
      }*/
    default:
      return false;
    }
  gettok();
  return true;
  }



// Primary
bool Expression::primary(FXdouble& result){
  if(token==TOK_PLUS || token==TOK_MINUS || token==TOK_NOT){
    Token t=token;
    gettok();
    if(!primary(result)) return false;
    if(t==TOK_MINUS) result=-result;
    else if(t==TOK_NOT) result=(FXdouble)(result==0.0);
    }
  else{
    if(!element(result)) return false;
    }
  return true;
  }


// Power expression
bool Expression::powexp(FXdouble& result){
  if(!primary(result)) return false;
  if(token==TOK_POWER){
    FXdouble w;
    gettok();
    if(!powexp(w)) return false;
    result=pow(result,w);
    }
  return true;
  }



// Mul expression
bool Expression::mulexp(FXdouble& result){
  if(!powexp(result)) return false;
  while(TOK_TIMES<=token && token<=TOK_MODULO){
    Token t=token;
    FXdouble w;
    gettok();
    if(!powexp(w)) return false;
    if(t==TOK_TIMES) result*=w;
    else if(t==TOK_DIVIDE) result/=w;
    else result=fmod(result,w);
    }
  return true;
  }


// Add expression
bool Expression::addexp(FXdouble& result){
  if(!mulexp(result)) return false;
  while(TOK_PLUS<=token && token<=TOK_MINUS){
    Token t=token;
    FXdouble w;
    gettok();
    if(!mulexp(w)) return false;
    if(t==TOK_MINUS) result+=w;
    else result-=w;
    }
  return true;
  }



// Compare expression
bool Expression::compexp(FXdouble& result){
  if(!addexp(result)) return false;
  if(TOK_LESS<=token && token<=TOK_NOTEQUAL){
    Token t=token;
    FXdouble w;
    gettok();
    if(!addexp(w)) return false;
    switch(t){
      case TOK_LESS: result=(result<w);  break;
      case TOK_LESSEQ: result=(result<=w); break;
      case TOK_GREATER: result=(result>w);  break;
      case TOK_GREATEREQ: result=(result>=w); break;
      case TOK_EQUAL: result=(result==w); break;
      default: result=(result!=w); break;
      }
    }
  return true;
  }


// Parse it
bool Expression::expr(FXdouble& result){
  if(!compexp(result)) return false;
  while(TOK_AND<=token && token<=TOK_XOR){
    Token t=token;
    FXdouble w;
    gettok();
    if(!compexp(w)) return false;
    if(t==TOK_AND) result=(result!=0.0)&(w!=0.0);
    else if(t==TOK_OR) result=(result!=0.0)|(w!=0.0);
    else result=(result!=0.0)^(w!=0.0);
    }
  return true;
  }


// Now, here's the beaf
bool evaluate(FXdouble& result,const FXchar* expression){
  Expression expr(expression);
  expr.gettok();
  return expr.expr(result);
  }

/*
  FXchar buffer[1000];
  FXdouble value=0.0;
  while(gets(buffer)){
    fprintf(stderr,"buf=%s\n",buffer);
    if(!evaluate(value,buffer)){ fprintf(stderr,"oops!\n"); }
    else{ fprintf(stderr,"value=%.10g\n",value); }
    }

*/

}
