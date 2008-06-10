/* tmaster.h
Includes the most used Tn library stuff
(C) 2001, 2002, 2003 Niall Douglas
*/

/* This code is licensed for use only in testing TnFOX facilities.
It may not be used by other code or incorporated into code without
prior written permission */

#ifndef _tmaster_h_
#define _tmaster_h_

#define FX_NO_GLOBAL_NAMESPACE
#include "fxdefs.h"

#include "FXRollback.h"
#include "FXStream.h"
#include "QTrans.h"
#include "QThread.h"
#include <assert.h>

/*! \namespace Tn
\brief Contains all Tn related code

Includes the FX namespace into itself so all TnFOX code appears within Tn
as well.
*/
namespace Tn {
using namespace FX;

/*! \file tmaster.h
\brief The master include file for the Tn library

This file sets up lots of macros and defines which all Tn code uses. Also included are
a stock set of TnFOX and Tn header files:
\li fxdefs.h
\li FXString.h
\li FXPolicies.h
\li FXException.h
\li FXGenericTools.h
\li FXRollback.h
\li FXProcess.h
\li FXStream.h
\li QTrans.h
\li QThread.h
\li \c <assert.h>

\li TKernelPath.h
\li TSimpleContainers.h

It also defines the Tn namespace and includes the FX namespace within it. Thus you
can directly refer to TnFOX members of the FX namespace without a qualifier.

Including any tornado header file will automatically include tmaster.h as they all use
items defined here.
*/

//! Maximum contents of a u8
#define U8_MAX 255
//! Maximum contents of a s8
#define S8_MAX 127
//! Maximum contents of a u16
#define U16_MAX 65535
//! Maximum contents of a s16
#define S16_MAX 32767
//! Maximum contents of a u32
#define U32_MAX 4294967295
//! Maximum contents of a s32
#define S32_MAX 2147483647
//! Maximum contents of a u64
#define U64_MAX 18446744073709551615
//! Maximum contents of a s64
#define S64_MAX 9223372036854775807

//! An unsigned eight bit container
typedef unsigned char u8;
//! A signed eight bit container
typedef signed char s8;
//! An unsigned sixteen bit container
typedef FXushort u16;
//! A signed sixteen bit container
typedef FXshort s16;
//! An unsigned thirty-two bit container
typedef FXuint u32;
//! A signed thirty-two bit container
typedef FXint s32;
//! An unsigned sixty-four bit container
typedef FXulong u64;
//! A signed sixty-four bit container
typedef FXlong s64;
//! An integer container big enough to hold a \c void *
typedef FXuval Tmval;
//! A future-proof offset into a file
typedef FXfval Tfval;

#ifdef _MSC_VER // MSVC
// Prevent daft warnings
#pragma warning(disable: 4251) // STL must have dll-interface to be used by this class (pointless as it's all templates)
#pragma warning(disable: 4275) // Non DLL interface class used as base for DLL interface class
#pragma warning(disable: 4355) // "this" used in base member init list
#pragma warning(disable: 4800) // forcing value to bool (pointless as there is no performance penalty)

#ifdef BUILDING_TCOMMON
#define TEXPORT_TCOMMON __declspec(dllexport)
#define TIMPORT_TCOMMON __declspec(dllimport)
#else
#define TEXPORT_TCOMMON __declspec(dllimport)
#define TIMPORT_TCOMMON __declspec(dllexport)
#endif
#ifdef BUILDING_TCLIENT
#define TEXPORT_TCLIENT __declspec(dllexport)
#define TIMPORT_TCLIENT __declspec(dllimport)
#else
#define TEXPORT_TCLIENT __declspec(dllimport)
#define TIMPORT_TCLIENT __declspec(dllexport)
#endif
#if !defined(BUILDING_TCOMMON) && !defined(BUILDING_TCLIENT)
// Building user code then
#define TEXPORT __declspec(dllexport)
#define TIMPORT __declspec(dllimport)
#else
#define TEXPORT __declspec(dllimport)
#define TIMPORT __declspec(dllexport)
#endif

#else // MSVC

#ifdef BUILDING_TCOMMON
#define TEXPORT_TCOMMON
#define TIMPORT_TCOMMON 
#else
#define TEXPORT_TCOMMON 
#define TIMPORT_TCOMMON
#endif
#ifdef BUILDING_TCLIENT
#define TEXPORT_TCLIENT
#define TIMPORT_TCLIENT
#else
#define TEXPORT_TCLIENT
#define TIMPORT_TCLIENT
#endif
#if !defined(BUILDING_TCOMMON) && !defined(BUILDING_TCLIENT)
// Building user code then
#define TEXPORT
#define TIMPORT
#else
#define TEXPORT
#define TIMPORT
#endif

#endif // MSVC

// Version information
//! Current version of the TClient library as a string
#define TVERSION "0.10"
//! Current version of the TClient library as an integer (major part)
#define TVERSION_MAJOR 0
//! Current version of the TClient library as an integer (minor part)
#define TVERSION_MINOR 10


//! The possible connection point types of kernel item
enum PointType
{
	Holder=0,	//!< A pure holder of other items (similar to a directory on other OS's)
	Input,		//!< An item which receives input
	Output,		//!< An item which produces output
	Data		//!< An item whose input is identical to its output and does no processing (ie; pure data)
};

//! Enumerates the exception codes used in Tn
enum TExceptionCodes
{
	TACCESSDENIEDCODE=FXEXCEPTION_USER		//!< A Tn::TAccessDenied error
};

/*! \class TAccessDenied
\brief An access denied error

*/
class TEXPORT_TCOMMON TAccessDenied : public FXException
{
public:
	//! Use TERRGNOACCESS() to instantiate
	TAccessDenied(const FXString &msg, FXuint flags)
		: FXException(0, 0, 0, msg, TACCESSDENIEDCODE, flags|FXERRH_ISINFORMATIONAL) { }
};
#define TERRGNOACCESS(msg, flags)		{ TAccessDenied e(msg, flags); FXERRH_THROW(e); }

} // namespace

// Default includes
//#include "TKernelPath.h"
//#include "TSimpleContainers.h"

#endif
