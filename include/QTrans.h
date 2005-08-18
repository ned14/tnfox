/********************************************************************************
*                                                                               *
*                 L a n g u a g e  T r a n s l a t i o n                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.                   *
*       NOTE THAT I DO NOT PERMIT ANY OF MY CODE TO BE PROMOTED TO THE GPL      *
*********************************************************************************
* This code is free software; you can redistribute it and/or modify it under    *
* the terms of the GNU Library General Public License v2.1 as published by the  *
* Free Software Foundation EXCEPT that clause 3 does not apply ie; you may not  *
* "upgrade" this code to the GPL without my prior written permission.           *
* Please consult the file "License_Addendum2.txt" accompanying this file.       *
*                                                                               *
* This code is distributed in the hope that it will be useful,                  *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                          *
*********************************************************************************
* $Id:                                                                          *
********************************************************************************/

#ifndef QTRANS_H
#define QTRANS_H

#include "fxdefs.h"
#include "FXProcess.h"

namespace FX {

/*! \file QTrans.h
\brief Defines classes and items used for human language translation
*/

class FXString;
class FXStream;

/*! \class QTransString
\brief Provides a delayed translation string

QTrans::tr() returns one of these which basically accumulates argument inserts
via arg() and then when asked to cast to FXString, actually performs the translation
based upon any parameter specialisations defined for the current locale in the
translation file.

QTransString also can convert to arbitrary language ids, but this functionality
is protected as QTransString has an implicit conversion to FXString. It can even
invoke a functor to return what language id to use, a facility used by Tn to return
Just-In-Time-Help strings to multiple clients in multiple languages in a thread safe
fashion. If you want this extended functionality, subclass QTransString.
*/
struct QTransStringPrivate;
class FXAPI QTransString
{
	friend class QTrans;
	friend struct QTransStringPrivate;
	QTransStringPrivate *p;
protected:
	QTransString() : p(0) { }
	QTransString(const char *context, const char *text, const char *hint, const FXString *langid=0);
	//! Returns true if the string is empty
	bool empty() const throw();
	//! Returns a copy of the string
	QTransString copy() const;
	//! The type of functor called to determine destination translation language
	typedef Generic::Functor<Generic::TL::create<const FXString *>::value> GetLangIdSpec;
	//! Gets the langid functor setting (=0 for none ie; use current user's)
	GetLangIdSpec *langIdFunc() const throw();
	//! Sets what to call to determine the destination translation language
	void setLangIdFunc(GetLangIdSpec *idfunc) throw();
	/*! Returns a translated FXString. Up until this point all operations are merely
	stored until this moment. */
	FXString translate(const FXString *id=0);
	//! Returns the context, text and hint
	void contents(const char *&context, const char *&text, const char *&hint) const;
public:
#ifndef HAVE_MOVECONSTRUCTORS
#ifdef HAVE_CONSTTEMPORARIES
	QTransString(const QTransString &o);
	QTransString &operator=(const QTransString &);
#else
	QTransString(QTransString &o);
	QTransString &operator=(QTransString &);
#endif
#else
private:
	QTransString(const QTransString &);	// disable copy constructor
public:
	QTransString(QTransString &&o);
#endif
	~QTransString();
	/*! Inserts an argument into the lowest numbered %x. Specifying a negative number
	for \em fieldwidth fills with zeros instead of spaces except for the string insert
	where negative numbers align to the left rather than right (as Qt does but for all
	these)
	*/
	QTransString &arg(const FXString &str, FXint fieldwidth=0);
	//! \overload
	QTransString &arg(char c, FXint fieldwidth=0);
	//! \overload
	QTransString &arg(FXlong num,  FXint fieldwidth=0, FXint base=10);
	//! \overload
	QTransString &arg(FXulong num, FXint fieldwidth=0, FXint base=10);
	//! \overload
	QTransString &arg(FXint num,  FXint fieldwidth=0, FXint base=10) { return arg((base!=10) ? (FXulong)((FXuint) num) : (FXlong) num, fieldwidth, base); }
	//! \overload
	QTransString &arg(FXuint num, FXint fieldwidth=0, FXint base=10) { return arg((FXulong) num, fieldwidth, base); }
	//! \overload
	QTransString &arg(double num, FXint fieldwidth=0, FXchar fmt='g', int prec=-1);
	/*! \overload */
	operator FXString() { return translate(); }

	//! Stores the translatable string's text plus id to identify this translation
	void save(FXStream &s) const;
	//! Loads the translatable string's text plus id to identify this translation
	void load(FXStream &s);
};


/*! \class QTrans
\brief The central class for human language translation (Qt compatible)

QTrans translates text displayed to the user to their preferred language, falling
back on English if no translation is available. To do this,
it reads in a file at startup containing translations from English to whatever
number of human languages - however the database can also be dynamically altered.

Usage of automatic human language conversion is very easy - simply wrap your
literal strings with tr() which returns the converted text as an QTransString. If you
use the %n insert identifiers and arg() it permits easy substitution of run-time
information into strings in a language neutral way ie; inserts can be reordered
based on accurate translation requirements. Furthermore, the translation file
can customise its translation based upon the values of the parameters - \link transfiletutorial
see the tutorial on writing translation files.
\endlink

Even easier use of tr() can occur in subclasses of FXObject as this
automatically adds the class name before calling tr() in QTrans for you (watch out
you don't call FXObject::tr() in your constructor - virtual tables haven't been set yet!).
In those classes which don't subclass FXObject, you must call the static method QTrans::tr()
manually specifying the class name.

The python script \link CppMunge
CppMunge.py
\endlink
can extract all code using tr() and place the strings
in a text file which you should call &lt;exename&gt;Trans.txt suitable for loading on startup. You
should extend each string definition with an appropriate translation and place it in
the same directory as the executable. You may optionally apply gzip to the file, thus
calling it &lt;exename&gt;Trans.gz and it will be automatically decompressed on load.
Alternatively you can embed the translation file into your executable using the
provided reswrap utility and QTRANS_SETTRANSFILE(). \link transfiletutorial
See the tutorial for more details.
\endlink

Note that QTrans is instantiated by FXProcess on startup. FXProcess provides special
command line support for QTrans so that the following command line
arguments when provided to the application have effect:
\li \c -fxtrans-language=&lt;ll&gt; where &lt;ll&gt; is a ISO639 two letter code sets that
language to override the locale-determined one. This is useful for testing.
\li \c -fxtrans-country=&lt;cc&gt; where &lt;cc&gt; is a ISO3166 two letter code sets that
region to override the locale-determined one. This is useful for testing.
\li \c -fxtrans-dump&lt;n&gt; causes the zero-based embedded translation file number <n> to
be dumped to stdout. You can save this, extend it and place it next to the
shared library as usual. Unfortunately, there is no way to know which number
is the library or application due to the vagaries of C++ static data
initialisation order (start with 0, increment from there).

The last two are intended for end-users who are not programmers to easily
extract and extend locale customisations of your application - even if they
only have the binaries.

\note All QTrans functions are thread-safe

<h4>Implementation notes:</h4>
As noted above, embedded data is registered statically with QTrans which then
loads it on FXProcess startup. However if QTrans is already created, it
immediately loads in the data - plus if embedded data is destructed, it
removes the data that was loaded from the particular containing module.

This means that you can embed translation files into your dynamically
loaded shared libraries and when you kick them in or out of memory it correctly
adds and removes the translations. QTrans determines which translation
bank it should preferentially use by checking if the address of the string
literal you wish to translate lies near to the static data initialised for
that binary - thus where each module defines a different translation of the
same English text, the correct one is found - however if a module doesn't
define its own translation, the first registered translation is used instead
(usually TnFOX itself, then the base executable - but it changes according
to how you have set up your FXProcess static init dependencies).
*/
template<class type> class QValueList;
class QTransPrivate;
struct QTransEmbeddedFile;
class FXAPI QTrans
{
	QTransPrivate *p;
	friend class QTransInit;
	QTrans(int &argc, char **argv, FXStream &txtout);
	FXDLLLOCAL void addData(QTransEmbeddedFile &data);
	FXDLLLOCAL void removeData(const QTransEmbeddedFile &data);
public:
	~QTrans();
	/*! \return The human language translated version of the text
	\param context Usually the classname of where you are calling from
	\param text The string to be translated in English. <b>This must be a literal</b> as
	its address is used to calculate which translation file to use.
	\param hint Usually unused, needed to resolve ambiguities eg; masculine/feminine genders
	when translating from gender neutral English. Never write "m" or "f" - instead if it refers
	to a box, use "box" - thus it doesn't matter if a box is masculine or feminine.

	Begins a translation of the text based upon the currently set user language.
	*/
	static QTransString tr(const char *context, const char *text, const char *hint=0);
	enum LanguageType
	{
		ISO639=0	//!< Two letter ISO639 standard
	};
	//! Returns the current language of the user currently using the application
	static const FXString &language(LanguageType=ISO639);
	enum CountryType
	{
		ISO3166=0	//!< Two letter ISO3166 standard
	};
	//! Returns the country the computer believes it is currently in
	static const FXString &country(CountryType=ISO3166);
	/*! Refreshes the internal cache of locale information. You should call this
	if you have reason to believe that the user's settings have changed. Note that
	the overrides taken precedence over any refreshes.
	*/
	static void refresh();
	/*! Overrides the language setting to the specified ISO639 two letter code.
	Specifying a null string returns the system to the user locale.
	*/
	static void overrideLanguage(const FXString &iso639);
	/*! Overrides the country setting to the specified ISO3166 two letter code.
	Specifying a null string returns the system to the user locale.
	*/
	static void overrideCountry(const FXString &iso3166);
	/*! \sa provided() */
	struct ProvidedInfo
	{
		FXString module, language, country;
		ProvidedInfo() { }
		ProvidedInfo(const FXString &_module, const FXString &_language, const FXString &_country)
			: module(_module), language(_language), country(_country) { }
	};
	//! Defines a list of QTrans::ProvidedInfo
	typedef QValueList<ProvidedInfo> ProvidedInfoList;
	/*! Returns a list of supported languages by each currently loaded library.
	Not every library will provide the same languages so to determine what
	language choices to offer your user you should only offer those provided
	by all libraries. The list is ready sorted by module first, then by language
	and finally country
	*/
	static ProvidedInfoList provided();
private:
	friend class QTrans_EmbeddedTranslationFile;
	friend class QTransString;
	FXDLLLOCAL void int_translateString(FXString &dest, QTransString &src, const FXString *langid=0);
	static void int_registerEmbeddedTranslationFile(const char *buffer, FXuint len, void *staticaddr);
	static void int_deregisterEmbeddedTranslationFile(const char *buffer, FXuint len, void *staticaddr);
};

class QTrans_EmbeddedTranslationFile
{
	const char *buffer;
	FXuint len;
public:
	QTrans_EmbeddedTranslationFile(const FXuchar *_buffer, FXuint _len) : buffer((const char *) _buffer), len(_len)
	{
		QTrans::int_registerEmbeddedTranslationFile(buffer, len, this);
	}
	QTrans_EmbeddedTranslationFile(const char *_buffer, FXuint _len) : buffer(_buffer), len(_len)
	{
		QTrans::int_registerEmbeddedTranslationFile(buffer, len, this);
	}
	~QTrans_EmbeddedTranslationFile()
	{
		QTrans::int_deregisterEmbeddedTranslationFile(buffer, len, this);
	}
};
/*! Registers a static char buffer to be an embedded translation file for the application.
*/
#define QTRANS_SETTRANSFILE(buffer, len) static QTrans_EmbeddedTranslationFile _tnfox_myembeddedtranslationfile_(buffer, len)

} // namespace

#endif
