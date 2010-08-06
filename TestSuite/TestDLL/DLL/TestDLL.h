#ifdef WIN32
#ifdef TESTDLL_EXPORTS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif
#else
#define EXPORT FXEXPORT
#endif

namespace Test {

class EXPORT TestDLL
{
public:
	TestDLL();
	~TestDLL();
	int test(int a);
};
}

extern "C" EXPORT Test::TestDLL *findDLL();
