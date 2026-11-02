// Differential tests of the error paths (issue #22): TASSERT, TERROR,
// PROGRAM_ERROR throws and the TEND wrapping of the original code against the
// reconstructed code, through imageeh.cpp.

#include "imageeh.h"
#include "../../src/Utils.h"

// Own random numbers (ER_SEED), so the later tests' inputs stay the same.
static uint32 ErSeed = 22;
static uint32 ErRandom()
{
	ErSeed = ErSeed * 1103515245 + 12345;
	return ErSeed >> 8;
}

static bool Trace = false; // ER_TRACE=1 prints every compared exception

// Compares what both sides threw.
static void Compare(const char *What, const TThrown &Ours, const TThrown &Original)
{
	if (Trace)
		printf("Errors %s: %s\n", What, Original.Describe().c_str());
	CHECK(Ours == Original, "Errors %s: ours %s, original %s", What, Ours.Describe().c_str(),
		Original.Describe().c_str());
}

typedef void (__cdecl *StrFn)(char *s);

static void TestUtils()
{
	Compare("LowerString(NULL)", OurThrow([] { LowerString(NULL); }), ImageThrow([] { At<StrFn>(0x5093d0)(NULL); }));
}

void TestErrors()
{
	Trace = getenv("ER_TRACE") != NULL && atoi(getenv("ER_TRACE")) != 0;
	if (getenv("ER_SEED") != NULL)
		ErSeed = strtoul(getenv("ER_SEED"), NULL, 10);
	int Before = Checks;
	EnableImageExceptions();
	TestUtils();
	DisableImageExceptions();
	printf("Errors: %d checks\n", Checks - Before);
}
