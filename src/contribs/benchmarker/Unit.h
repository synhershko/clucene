#pragma once
#include "CLucene/util/Misc.h"


class Unit
{
public:
	void start(Benchmarker* benchmarker);
	void stop();
	virtual const char* getName()=0;
	Unit();
protected:
	Timer timerCase;
	Timer timerTotal;
	int testsCountTotal;
	int testsCountSuccess;
	int testsRunTotal;
	int testsRunSuccess;
	Benchmarker* bm;

	void runTest(const char* testName,LPTEST_ROUTINE func, int iterations);
	virtual void runTests()=0;
};
