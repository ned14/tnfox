/********************************************************************************
*                                                                               *
*                          G e n e r i c   A r r a y                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXArray.h,v 1.18 2005/01/16 16:06:06 fox Exp $                           *
********************************************************************************/
#ifndef FXARRAY_H
#define FXARRAY_H

#ifndef FXELEMENT_H
#include "FXElement.h"
#endif

namespace FX {


/*************************  D e f i n i t i o n  *******************************/


template<class TYPE>
class FXArray {
  TYPE   *list;                // List of items
  FXint   number;              // Used slots
  FXint   total;               // Total slots
  FXint   grow;                // Grow amount
public:
  FXArray();
  FXArray(const FXArray<TYPE>& orig);
  FXArray<TYPE>& operator=(const FXArray<TYPE> &orig);
  inline FXint no() const;
  inline TYPE* data() const;
  inline FXint size() const;
  void size(FXint n);
  FXint inc() const;
  void inc(FXint n);
  inline const TYPE& operator [](FXint i) const;
  inline TYPE& operator [](FXint i);
  void insert(FXint pos,const TYPE& p);
  void append(const TYPE& p);
  void prepend(const TYPE& p);
  void remove(FXint pos);
  void extract(const TYPE& p);
  FXint find(const TYPE& p,FXint pos=0);
  FXint rfind(const TYPE& p,FXint pos=2147483647);
  void trunc();
  void clear();
  void save(FXStream& store) const;
  void load(FXStream& store);
 ~FXArray();
  };


/**********************  I m p l e m e n t a t i o n  ************************/


// Construct as empty
template<class TYPE>
FXArray<TYPE>::FXArray(){
  list=NULL;
  number=0;
  total=0;
  grow=0;
  }


// Copy construct
template<class TYPE>
FXArray<TYPE>::FXArray(const FXArray<TYPE>& orig){
  number=orig.number;
  total=orig.total;
  grow=orig.grow;
  allocElms(list,total);
  constructElms(list,number);
  copyElms(list,orig.list,number);
  }


// Return number of elements
template<class TYPE>
FXint FXArray<TYPE>::no() const {
  return number;
  }


// Return pointer to the list
template<class TYPE>
TYPE* FXArray<TYPE>::data() const {
  return list;
  }


// Return size of list
template<class TYPE>
FXint FXArray<TYPE>::size() const {
  return total;
  }


// Return grow delta
template<class TYPE>
FXint FXArray<TYPE>::inc() const {
  return grow;
  }


// Set grow delta
template<class TYPE>
void FXArray<TYPE>::inc(FXint g){
  FXASSERT(g>=0);
  grow=g;
  }


// Return element rvalue
template<class TYPE>
const TYPE& FXArray<TYPE>::operator[](FXint i) const {
  FXASSERT(0<=i&&i<number);
  return list[i];
  }


// Return element lvalue
template<class TYPE>
TYPE& FXArray<TYPE>::operator[](FXint i){
  FXASSERT(0<=i&&i<number);
  return list[i];
  }


// Assign an array
template<class TYPE>
FXArray<TYPE>& FXArray<TYPE>::operator=(const FXArray<TYPE>& orig){
  if(this!=&orig){
    destructElms(list,number);
    freeElms(list);
    number=orig.number;
    total=orig.total;
    grow=orig.grow;
    allocElms(list,total);
    constructElms(list,number);
    copyElms(list,orig.list,number);
    }
  return *this;
  }


// Set new size
template<class TYPE>
void FXArray<TYPE>::size(FXint n){
  FXASSERT(n>=0);
  if(n!=number){
    if(n<number){
      destructElms(&list[n],number-n);                // Destruct old elements
      if(n==0){
        freeElms(list);
        total=0;
        }
      }
    else{
      if(n>total){
        TYPE *ptr;
        FXint s=total+grow;
        if(grow==0) s=total+total;
        if(s<n) s=n;
        allocElms(ptr,s);
        constructElms(ptr,n);                         // Construct blank elements
        copyElms(ptr,list,number);                    // Uses assignment operator
        destructElms(list,number);                    // Destruct old ones
        freeElms(list);
        list=ptr;
        total=s;
        }
      FXASSERT(n<=total);
      constructElms(&list[number],n-number);          // Construct new elements
      }
    number=n;
    }
  }


// Insert element anywhere
template<class TYPE>
void FXArray<TYPE>::insert(FXint pos,const TYPE& p){
  FXint s=number-pos;
  size(number+1);
  moveElms(&list[pos+1],&list[pos],s);
  list[pos]=p;
  }


// Append element at end
template<class TYPE>
void FXArray<TYPE>::append(const TYPE& p){
  FXint s=number;
  size(number+1);
  list[s]=p;
  }


// Prepend element at begin
template<class TYPE>
void FXArray<TYPE>::prepend(const TYPE& p){
  FXint s=number;
  size(number+1);
  moveElms(&list[1],&list[0],s);
  list[0]=p;
  }


// Remove element at pos
template<class TYPE>
void FXArray<TYPE>::remove(FXint pos){
  FXint s=number;
  moveElms(&list[pos],&list[pos+1],s-pos-1);          // Uses assignment operator
  size(s-1);
  }


// Find element, -1 if not found
template<class TYPE>
FXint FXArray<TYPE>::find(const TYPE& p,FXint pos){
  if(pos<0) pos=0;
  while(pos<number){ if(list[pos]==p){ return pos; } ++pos; }
  return -1;
  }


// Find element, -1 if not found
template<class TYPE>
FXint FXArray<TYPE>::rfind(const TYPE& p,FXint pos){
  if(pos>=number) pos=number-1;
  while(0<=pos){ if(list[pos]==p){ return pos; } --pos; }
  return -1;
  }


// Extract element
template<class TYPE>
void FXArray<TYPE>::extract(const TYPE& p){
  FXint s=number;
  while(s-- != 0){
    if(list[s]==p){
      moveElms(&list[s],&list[s+1],number-s);         // Uses assignment operator
      size(number-1);
      break;
      }
    }
  }


// Trunc excess
template<class TYPE>
void FXArray<TYPE>::trunc(){
  size(number);
  }


// Clear the list
template<class TYPE>
void FXArray<TYPE>::clear(){
  size(0);
  }


// Save operator
template<class TYPE>
void FXArray<TYPE>::save(FXStream& store) const {
  store << number << total << grow;
  saveElms(store,list,number);                        // Uses save operator
  }


// Load operator
template<class TYPE>
void FXArray<TYPE>::load(FXStream& store){
  destructElms(list,number);                          // Destruct elements
  freeElms(list);
  store >> number >> total >> grow;
  allocElms(list,total);
  constructElms(list,number);                         // Fresh elements
  loadElms(store,list,number);                        // Uses load operator
  }


// Destruct list
template<class TYPE>
FXArray<TYPE>::~FXArray(){
  destructElms(list,number);
  freeElms(list);
  }

}

#endif

