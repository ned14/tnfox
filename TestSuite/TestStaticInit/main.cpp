/********************************************************************************
*                                                                               *
*                      Test of FXProcess static init                            *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.            *
*   NOTE THAT I NIALL DOUGLAS DO NOT PERMIT ANY OF MY CODE USED UNDER THE GPL   *
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

#include "fx.h"

template<char name, int version> class Init
{
public:
	Init() { fxmessage("%c%d\n", name, version); }
};

typedef Init<'A',1> A1;
typedef Init<'A',2> A2;
typedef Init<'B',1> B1;
typedef Init<'B',2> B2;
typedef Init<'B',3> B3;
typedef Init<'C',1> C1;
typedef Init<'C',2> C2;
typedef Init<'C',3> C3;
typedef Init<'C',4> C4;

static FXProcess_StaticInit<C4> C4init("C4");
FXPROCESS_STATICDEP(C4init, "A2");
FXPROCESS_STATICDEP(C4init, "B1");
FXPROCESS_STATICDEP(C4init, "C2");
static FXProcess_StaticInit<B2> B2init("B2");
FXPROCESS_STATICDEP(B2init, "B3");
static FXProcess_StaticInit<C3> C3init("C3");
static FXProcess_StaticInit<A1> A1init("A1");
FXPROCESS_STATICDEP(A1init, "C3");
FXPROCESS_STATICDEP(A1init, "B1");
static FXProcess_StaticInit<C2> C2init("C2");
static FXProcess_StaticInit<B1> B1init("B1");
FXPROCESS_STATICDEP(B1init, "C2");
static FXProcess_StaticInit<B3> B3init("B3");
FXPROCESS_STATICDEP(B3init, "C4");
static FXProcess_StaticInit<C1> C1init("C1");
static FXProcess_StaticInit<A2> A2init("A2");
FXPROCESS_STATICDEP(A2init, "A1");

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	fxmessage("\nTest of static init facility\nLetters above should be:\n");
	fxmessage("       C1        C2        C3  \n");
	fxmessage("                  | \\       |  \n");
	fxmessage("                  B1  \\     | \n");
	fxmessage("                    \\\\-\\----A1 \n");
	fxmessage("                     \\  \\   |  \n");
	fxmessage("                      \\ |  A2  \n");
	fxmessage("                       \\| /    \n");
	fxmessage("                        C4     \n");
	fxmessage("                        |      \n");
	fxmessage("                        B3     \n");
	fxmessage("                        |      \n");
	fxmessage("                        B2     \n");

#ifdef _MSC_VER
	getchar();
#endif
	return 0;
}
