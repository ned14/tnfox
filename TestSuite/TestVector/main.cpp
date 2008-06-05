/********************************************************************************
*                                                                               *
*                                  Vector test                                  *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2008 by Niall Douglas.   All Rights Reserved.            *
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

#include "FXMaths.h"
using namespace FX;
using namespace FX::Maths;

// 72,000 floats is ~282Kb. Ensure six of these stay inside a L2 cache!
static const FXuint TOTALITEMS=72000;
namespace FP
{
	typedef VectorArray<float, 18, TOTALITEMS/18> SlowArray;
	typedef VectorArray<float, 16, TOTALITEMS/16> FastArray;
}
namespace Int
{
	typedef VectorArray<int, 18, TOTALITEMS/18> SlowArray;
	typedef VectorArray<int, 16, TOTALITEMS/16> FastArray;
}

template<class type> static void fill(type *FXRESTRICT array, typename type::BASETYPE *FXRESTRICT what)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::VECTORTYPE); n++)
		{
			(*array)[n]=typename type::VECTORTYPE(what);
		}
	}
}

template<class type> static void sqrtmulacc(type *FXRESTRICT dest, type *FXRESTRICT A, type *FXRESTRICT B)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::VECTORTYPE); n++)
		{
			typename type::VECTORTYPE &d=(*dest)[n], &a=(*A)[n], &b=(*B)[n];
			d=sqrt(d+a*b);
		}
	}
}

template<class type> static void acc(type *FXRESTRICT dest, type *FXRESTRICT A, type *FXRESTRICT B)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::VECTORTYPE); n++)
		{
			typename type::VECTORTYPE &d=(*dest)[n], &a=(*A)[n], &b=(*B)[n];
			d=d+a+b;
		}
	}
}

template<class type> static void dotsum(type *FXRESTRICT dest, type *FXRESTRICT A, type *FXRESTRICT B)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::VECTORTYPE); n++)
		{
			(*dest)[n].set(0, dot((*A)[n], (*B)[n]));
			(*dest)[n].set(1, sum((*A)[n])+sum((*B)[n]));
		}
	}
}

template<class type> static void logic(type *FXRESTRICT dest, type *FXRESTRICT A, type *FXRESTRICT B)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::VECTORTYPE); n++)
		{
			(*dest)[n]=(*dest)[n] ^ ((*A)[n]|(*B)[n]);
		}
	}
}

template<class type> static void compare(type *FXRESTRICT dest, type *FXRESTRICT A, type *FXRESTRICT B)
{
	//for(FXuint a=0; a<1000; a++)
	{
		for(FXuint n=0; n<4*TOTALITEMS/sizeof(typename type::VECTORTYPE); n++)
		{
			(*dest)[n]=(*dest)[n] >= (*A)[n] || (*dest)[n] <= (*B)[n] || !((*dest)[n] != (*A)[n]);
		}
	}
}

static double Time(const char *desc, const Generic::BoundFunctorV &_routine)
{
	Generic::BoundFunctorV &routine=(Generic::BoundFunctorV &) _routine;
	FXulong start=FXProcess::getNsCount();
	for(FXuint a=0; a<1000; a++)
		routine();
	FXulong end=FXProcess::getNsCount();
	double ret=65536000000000.0/(end-start);
	fxmessage("%s performed %f ops/sec\n", desc, ret);
	return ret;
}

template<typename A, typename B> static bool verify(A &a, B &b)
{
	for(FXuint n=0; n<TOTALITEMS/18; n++)
		if(a[n][0]!=b[n][0] || a[n][1]!=b[n][1] || a[n][2]!=b[n][2] || a[n][3]!=b[n][3])
			return false;
	return true;
}

int main( int argc, char *argv[] )
{
	double slowt, fastt;
	FXProcess myprocess(argc, argv);
	fxmessage("Floating-Point SIMD vector tests\n"
		      "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
	{	// FP tests
		using namespace FP;
		static SlowArray slowA, slowB, slowC;
		static FastArray fastA, fastB, fastC;
		float foo1[]={ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
		float foo2[]={ 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, 2.0f };
		fill(&slowA, foo1);
		fill(&fastA, foo1);
		fill(&slowB, foo1);
		fill(&fastB, foo1);
		fastt=Time("   vector<16>", Generic::BindFunc(fill<FastArray>, &fastC, foo2));
		slowt=Time("   vector<18>", Generic::BindFunc(fill<SlowArray>, &slowC, foo2));
		fxmessage("Filling vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		// Arithmetic test
		fastt=Time("   vector<16>", Generic::BindFunc(sqrtmulacc<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(sqrtmulacc<SlowArray>, &slowC, &slowA, &slowB));
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Arithmetic failed!\n"); return 1; }
		fxmessage("Square-Root of Multiply-Accumulate vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		// Comparison test
		fastt=Time("   vector<16>", Generic::BindFunc(compare<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(compare<SlowArray>, &slowC, &slowA, &slowB));
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Comparisons failed!\n"); return 1; }
		fxmessage("Comparisons vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		// Dot & sum test
		//slowt=Time("   vector<5>", Generic::BindFunc(dotsum<SlowArray>, &slowC, &slowA, &slowB));
		//fastt=Time("   vector<4>", Generic::BindFunc(dotsum<FastArray>, &fastC, &fastA, &fastB));
		//if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Comparisons failed!\n"); return 1; }
		//fxmessage("Dotsum vector<4> was %f times faster than vector<5>\n\n", fastt/slowt);
	}
	fxmessage("\n\nInteger SIMD vector tests\n"
		          "-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	{	// Logical test
		using namespace Int;
		static SlowArray slowA, slowB, slowC;
		static FastArray fastA, fastB, fastC;
		int foo1[]={ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
		int foo2[]={ 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119 };
		fill(&slowA, foo1);
		fill(&fastA, foo1);
		fill(&slowB, foo1);
		fill(&fastB, foo1);
		fastt=Time("   vector<16>", Generic::BindFunc(fill<FastArray>, &fastC, foo2));
		slowt=Time("   vector<18>", Generic::BindFunc(fill<SlowArray>, &slowC, foo2));
		fxmessage("Filling vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		// Arithmetic test
		fastt=Time("   vector<16>", Generic::BindFunc(acc<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(acc<SlowArray>, &slowC, &slowA, &slowB));
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Arithmetic failed!\n"); return 1; }
		fxmessage("Accumulate vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		// Comparison test
		fastt=Time("   vector<16>", Generic::BindFunc(compare<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(compare<SlowArray>, &slowC, &slowA, &slowB));
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Comparisons failed!\n"); return 1; }
		fxmessage("Comparisons vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);

		fastt=Time("   vector<16>", Generic::BindFunc(logic<FastArray>, &fastC, &fastA, &fastB));
		slowt=Time("   vector<18>", Generic::BindFunc(logic<SlowArray>, &slowC, &slowA, &slowB));
		if(!verify(slowC, fastC)) { fxmessage("Buffer contents not the same! Logical failed!\n"); return 1; }
		fxmessage("Logic vector<16> was %f times faster than vector<18>\n\n", fastt/slowt);
	}

	fxmessage("All Done!\n");
#ifdef _MSC_VER
	if(!myprocess.isAutomatedTest())
		getchar();
#endif
	return 0;
}
