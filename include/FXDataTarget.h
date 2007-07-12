/********************************************************************************
*                                                                               *
*                              D a t a   T a r g e t                            *
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
* $Id: FXDataTarget.h,v 1.25 2006/01/22 17:58:00 fox Exp $                      *
********************************************************************************/
#ifndef FXDATATARGET_H
#define FXDATATARGET_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif
#include <qptrvector.h>

namespace FX {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

/**
* A Data Target allows a valuator widget such as a Slider or Text Field
* to be directly connected with a variable in the program.
* Whenever the valuator control changes, the variable connected through
* the data target is automatically updated; conversely, whenever the program
* changes a variable, all the connected valuator widgets will be updated
* to reflect this new value on the display.
* Data Targets also allow connecting Radio Buttons, Menu Commands, and so on
* to a variable.  In this case, the new value of the connected variable is computed
* by subtracting ID_OPTION from the message ID.
*
* TnFOX adds an upcall mechanism whereby any number of FX::Generic::BoundFunctorV's
* may be attached to a data target which are upcalled when the value changes. This
* is particularly useful when combined with FX::FXDataTargetI for generating generic
* code not requiring subclassing FXObject (which is HARD in a template)
*/
class FXAPI FXDataTarget : public FXObject {
  FXDECLARE(FXDataTarget)
protected:
  FXObject     *target;                 // Target object
  void         *data;                   // Associated data
  FXSelector    message;                // Message ID
  FXuint        type;                   // Type of data
  QPtrVector<Generic::BoundFunctorV> upcalls;
private:
  FXDataTarget(const FXDataTarget&);
  FXDataTarget& operator=(const FXDataTarget&);
public:
  long onCmdValue(FXObject*,FXSelector,void*);
  long onUpdValue(FXObject*,FXSelector,void*);
  long onCmdOption(FXObject*,FXSelector,void*);
  long onUpdOption(FXObject*,FXSelector,void*);
public:
  enum {
    DT_VOID=0,
    DT_CHAR,
    DT_UCHAR,
    DT_SHORT,
    DT_USHORT,
    DT_INT,
    DT_UINT,
    DT_LONG,
    DT_ULONG,
    DT_FLOAT,
    DT_DOUBLE,
    DT_STRING,
    DT_LAST
    };
public:
  enum {
    ID_VALUE=1,                   /// Will cause the FXDataTarget to ask sender for value
    ID_OPTION=ID_VALUE+10001,     /// ID_OPTION+i will set the value to i where -10000<=i<=10000
    ID_LAST=ID_OPTION+10000
    };
public:

  /// Associate with nothing
  FXDataTarget():target(NULL),data(NULL),message(0),type(DT_VOID){}

  /// Associate with nothing
  FXDataTarget(FXObject* tgt,FXSelector sel):target(tgt),data(NULL),message(sel),type(DT_VOID){}

  /// Associate with character variable
  FXDataTarget(FXchar& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_CHAR),upcalls(true){}

  /// Associate with unsigned character variable
  FXDataTarget(FXuchar& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_UCHAR),upcalls(true){}

  /// Associate with signed short variable
  FXDataTarget(FXshort& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_SHORT),upcalls(true){}

  /// Associate with unsigned short variable
  FXDataTarget(FXushort& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_USHORT),upcalls(true){}

  /// Associate with int variable
  FXDataTarget(FXint& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_INT),upcalls(true){}

  /// Associate with unsigned int variable
  FXDataTarget(FXuint& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_UINT),upcalls(true){}

  /// Associate with long variable
  FXDataTarget(FXlong& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_LONG){}

  /// Associate with unsigned long variable
  FXDataTarget(FXulong& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_ULONG){}

  /// Associate with float variable
  FXDataTarget(FXfloat& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_FLOAT),upcalls(true){}

  /// Associate with double variable
  FXDataTarget(FXdouble& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_DOUBLE),upcalls(true){}

  /// Associate with string variable
  FXDataTarget(FXString& value,FXObject* tgt=NULL,FXSelector sel=0):target(tgt),data(&value),message(sel),type(DT_STRING),upcalls(true){}


  /// Set the message target object for this data target
  void setTarget(FXObject *t){ target=t; }

  /// Get the message target object for this data target, if any
  FXObject* getTarget() const { return target; }


  /// Set the message identifier for this data target
  void setSelector(FXSelector sel){ message=sel; }

  /// Get the message identifier for this data target
  FXSelector getSelector() const { return message; }


  /// Return type of data its connected to
  FXuint getType() const { return type; }

  /// Return pointer to data its connected to
  void* getData() const { return data; }


  /// Adds a bound functor to be called when the data changes
  Generic::BoundFunctorV *addUpcall(FXAutoPtr<Generic::BoundFunctorV> upcall);

  /// Removes a bound functor being called when the data changes
  bool removeUpcall(Generic::BoundFunctorV *upcall);


  /// Associate with nothing
  void connect(){ data=NULL; type=DT_VOID; }

  /// Associate with character variable
  void connect(FXchar& value){ data=&value; type=DT_CHAR; }

  /// Associate with unsigned character variable
  void connect(FXuchar& value){ data=&value; type=DT_UCHAR; }

  /// Associate with signed short variable
  void connect(FXshort& value){ data=&value; type=DT_SHORT; }

  /// Associate with unsigned short variable
  void connect(FXushort& value){ data=&value; type=DT_USHORT; }

  /// Associate with int variable
  void connect(FXint& value){ data=&value; type=DT_INT; }

  /// Associate with unsigned int variable
  void connect(FXuint& value){ data=&value; type=DT_UINT; }

  /// Associate with long variable
  void connect(FXlong& value){ data=&value; type=DT_LONG; }

  /// Associate with unsigned long variable
  void connect(FXulong& value){ data=&value; type=DT_ULONG; }

  /// Associate with float variable
  void connect(FXfloat& value){ data=&value; type=DT_FLOAT; }

  /// Associate with double variable
  void connect(FXdouble& value){ data=&value; type=DT_DOUBLE; }

  /// Associate with string variable
  void connect(FXString& value){ data=&value; type=DT_STRING; }


  /// Associate with nothing; also set target and message
  void connect(FXObject* tgt,FXSelector sel){ target=tgt; data=NULL; message=sel; type=DT_VOID; }

  /// Associate with character variable; also set target and message
  void connect(FXchar& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_CHAR; }

  /// Associate with unsigned character variable; also set target and message
  void connect(FXuchar& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_UCHAR; }

  /// Associate with signed short variable; also set target and message
  void connect(FXshort& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_SHORT; }

  /// Associate with unsigned short variable; also set target and message
  void connect(FXushort& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_USHORT; }

  /// Associate with int variable; also set target and message
  void connect(FXint& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_INT; }

  /// Associate with unsigned int variable; also set target and message
  void connect(FXuint& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_UINT; }

  /// Associate with long variable; also set target and message
  void connect(FXlong& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_LONG; }

  /// Associate with unsigned long variable; also set target and message
  void connect(FXulong& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_ULONG; }

  /// Associate with float variable; also set target and message
  void connect(FXfloat& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_FLOAT; }

  /// Associate with double variable; also set target and message
  void connect(FXdouble& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_DOUBLE; }

  /// Associate with string variable; also set target and message
  void connect(FXString& value,FXObject* tgt,FXSelector sel){ target=tgt; data=&value; message=sel; type=DT_STRING; }


  /// Destroy
  virtual ~FXDataTarget();
  };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/**
* A data target with storage
*/
template<typename T> class FXDataTargetI : public FXDataTarget
{
  T mydata;
  void callTarget() { if(target) target->handle(this,FXSEL(SEL_COMMAND,message),data); }
public:
  /// Constructs an instance targeting the specified item
  FXDataTargetI(FXObject* tgt=NULL, FXSelector sel=0) : FXDataTarget(mydata, tgt, sel) { }

  /// Retrieves the value
  const T& operator *() const { return mydata; }

  friend struct Returner;
  /// Strawman structure to manage setting
  struct Returner {
    FXDataTargetI& dt;
	Returner(const Returner &o) : dt(o.dt) { }
	const T& operator=(const T& val) {
      dt.mydata=val;
      dt.callTarget();
      return dt.mydata;
    }
	const T& operator=(const Returner &val) { return operator=(static_cast<const T &>(val)); }
	operator T&() { return dt.mydata; }
    operator const T&() const { return dt.mydata; }
  private:
    friend class FXDataTargetI;
    Returner(FXDataTargetI& _dt) : dt(_dt) { }
    };

  /// Permits retrieval or setting of the value
  Returner operator *() { return Returner(*this); }
  };

}

#endif

