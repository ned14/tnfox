/********************************************************************************
*                                                                               *
*                       Test of generic tools facilities                        *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2007 by Niall Douglas.   All Rights Reserved.       *
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

#include "fx.h"

#define UNIQUENAME3(var, no) var##no
#define UNIQUENAME2(var, no) UNIQUENAME3(var, no)
#define UNIQUENAME(var) UNIQUENAME2(var, __LINE__)

class Base { };
class Derived : public Base { };
namespace FX { namespace Generic { namespace ClassTraits {
	template<> struct has<Base> : public combine<POD> {};
}}}

template<typename T> void PrintN()
{
	fxmessage("%s = %d\n", Generic::typeInfo<T>().name().text(), T::value);
}
template<typename T> void PrintT()
{
	fxmessage("%s = %s\n", Generic::typeInfo<T>().name().text(), Generic::typeInfo<typename T::value>().name().text());
}

template<typename T> struct type { };

#define FNINFO(tag) fxmessage("%s = %d\n", #tag, Finfo::tag)
#define FNINFOT(tag) fxmessage("%s = %s\n", #tag, Generic::typeInfo<type<typename Finfo::tag> >().name().text())
#define FNINFOP(n) PrintPar< n<=Finfo::arity, n, partype##n<Finfo> > par##n(0)
template<bool print, int idx, typename par> struct PrintPar { PrintPar(int) { } };
template<int idx, typename par> struct PrintPar<true, idx, par> { PrintPar(int) {
	fxmessage("P%d = %s\n", idx, Generic::typeInfo<type<typename par::value> >().name().text()); } };
#define FNINFODEFPAR(n) template<class Finfo> struct partype##n { typedef typename Finfo::par##n##Type value; }
FNINFODEFPAR(1);
FNINFODEFPAR(2);
FNINFODEFPAR(3);
FNINFODEFPAR(4);
template<typename F> void doFnInfo(F fn)
{
	fxmessage("\nFunction is: %s (0x%08p)\n", Generic::typeInfo<F>().name().text(), fn);
	typedef typename Generic::FnInfo<F> Finfo;
	FNINFO(arity);
	FNINFO(isConst);
	FNINFOT(resultType);
	FNINFOT(objectType);
	FNINFOP(1);
	FNINFOP(2);
	FNINFOP(3);
	FNINFOP(4);
}

#define TRV(tag) fxmessage("%s = %d\n", #tag, Tinfo::tag)
#define TRT(tag) fxmessage("%s = %s\n", #tag, Generic::typeInfo<type<typename Tinfo::tag> >().name().text())
template<typename T> void doTraits()
{
	fxmessage("\nType is: %s\n", Generic::typeInfo<type<T> >().name().text());
	typedef typename Generic::Traits<T> Tinfo;
	TRV(isVoid); TRV(isPtr); TRV(isRef); TRV(isArray); TRV(isPtrToCode); TRV(isMemberPtr); TRV(isFunctionPtr);
	TRV(isValue); TRV(isIndirect); TRV(isConst); TRV(isVolatile);

	TRV(isFloat); TRV(isInt); TRV(isSigned); TRV(isUnsigned); TRV(isArithmetical); TRV(isIntegral);
	TRV(isBasic); TRV(holdsData); TRV(isEnum); TRV(isPolymorphic); TRV(isPOD);

	TRT(baseType);
	TRT(asRWParam);
	TRT(asConstParam);
	TRT(asROParam);
}
template<typename T> void doTraits2(T type)
{
	doTraits<T>();
}

static float testfunction(int a, int b)
{
	float ret=(float)((a+b)*2.23);
	fxmessage("Test function called, result is %f\n", ret);
	return ret;
}
struct TestStruct
{
	int foo(int a)
	{
		return a+1;
	}
};

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	fxmessage("TnFOX Generic Tools test:\n"
		      "-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	// Value
	PrintN<Generic::sameType<int, int> >();
	PrintN<Generic::sameType<int, double> >();
	PrintN<Generic::sameType<Generic::IntToType<6*3>, Generic::IntToType<9*2> > >();
	PrintT<Generic::select<(sizeof(257)>8), short, char> >();
	PrintN<Generic::convertible<double, int> >();
	PrintN<Generic::convertible<void *, FXuval> >();
	PrintN<Generic::convertible<void *, Base *> >();
	PrintN<Generic::convertible<FXProcess *, char> >();
	PrintN<Generic::convertible<const FXProcess &, FXProcess &> >();
	PrintN<Generic::convertible<Base *, Derived *> >();
	PrintN<Generic::convertible<Base **, Derived **> >();
	PrintN<Generic::convertible<void, void *> >();
	PrintN<Generic::convertible<void, void> >();
	PrintN<Generic::indirs<int ****> >();
	PrintN<Generic::indirs<int ****&> >();
	PrintN<Generic::indirs<const int &> >();
	PrintN<Generic::indirs<int> >();
	// Types
	PrintT<Generic::lessIndir<int **> >();
	PrintT<Generic::lessIndir<int *&> >();
	PrintT<Generic::lessIndir<int> >();
	PrintT<Generic::lessIndir<const volatile int **> >();
	PrintT<Generic::leastIndir<int ****> >();
	PrintT<Generic::leastIndir<int **&> >();
	PrintT<Generic::leastIndir<void> >();
	PrintT<Generic::addRef<int> >();
	PrintT<Generic::addRef<int *> >();
	PrintT<Generic::addRef<int *&> >();
	PrintT<Generic::addRef<void> >();
	// Typelists
	typedef Generic::IntegralLists::Ints list;
	fxmessage("List of ints is: %s\n", Generic::typeInfo<list>().name().text());
	PrintN<Generic::TL::length<list> >();
	PrintT<Generic::TL::at<list, 2> >();
	PrintT<Generic::TL::at<list, 9999> >();
	PrintN<Generic::TL::find<list, unsigned long> >();
	PrintT<Generic::TL::append<list, FXProcess &> >();
	PrintT<Generic::TL::remove<list, int> >();
	// Instantiated typelists
	typedef Generic::TL::create<int, float, int>::value ilist;
	Generic::TL::instantiateH<ilist> ilistcont;
	Generic::TL::instance<1>(ilistcont).value=78.0;
	// Next line should cause a warning
	Generic::TL::instance<0>(ilistcont).value=Generic::TL::instance<1>(ilistcont).value+1;
	Generic::TL::instance<2>(ilistcont).value=192;
	fxmessage("Instantiated list (length %d) is: %d, %f, %d\n",
		sizeof(ilistcont),
		Generic::TL::instance<0>(ilistcont).value,
		Generic::TL::instance<1>(ilistcont).value,
		Generic::TL::instance<2>(ilistcont).value);
	const Generic::TL::instantiateH<ilist> &ilistcontc=ilistcont;
	fxmessage("Const instantiated list is: %d, %f, %d\n",
		Generic::TL::instance<0>(ilistcontc).value,
		Generic::TL::instance<1>(ilistcontc).value,
		Generic::TL::instance<2>(ilistcontc).value);
	//Generic::TL::instance<1>(ilistcontc).value=78.0;
	// Function info
	doFnInfo(main);
	doFnInfo(&FXProcess::instance);
	doFnInfo(&FXException::sourceInfo);
	doFnInfo(&FXArrowButton::onPaint);
	// Traits
	doTraits<int>();
	doTraits<unsigned char *&>();
	doTraits<double[10]>();
	doTraits2(main);
	doTraits2(FXProcess::instance());
	doTraits<FXModality>();
	doTraits<FXObject>();
	doTraits<Base>();
	// Functors
	fxmessage("\n");
	typedef Generic::TL::create<float, int, int>::value testfnsig;
	Generic::Functor<testfnsig> testfn(testfunction);
	fxmessage("Functor call return was %f, size was %d\n", testfn(4,7), sizeof(testfn));
	Generic::BoundFunctor<testfnsig> *testfnb=Generic::BindFunctorN(testfn, 2, 5);
	fxmessage("Bound Functor return was %f, size was %d\n", (*testfnb)(), sizeof(*testfnb));
	//Generic::BoundFunctorV *testfnbv=(testfnb);
	//testfnbv();
	Generic::BoundFunctorV *testfnbv2=Generic::BindFuncN(testfunction, 4, 7);
	(*testfnbv2)();
	TestStruct teststruct;
	Generic::BoundFunctor<Generic::TL::create<int, int>::value> testfnb2=Generic::BindObj(teststruct, &TestStruct::foo, 5);
	fxmessage("Bound object functor call result=%d\n", testfnb2());

	fxmessage("All Done!\n");
#ifdef _MSC_VER
	if(!myprocess.isAutomatedTest())
		getchar();
#endif
	return 0;
}
