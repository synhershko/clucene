#pragma once

class Benchmarker
{
	lucene::util::CLVector<Unit*> tests;
public:
	Timer timerTotal;
	int testsCountTotal;
	int testsCountSuccess;
	int testsRunTotal;
	int testsRunSuccess;

	Benchmarker(void);
	void Add(Unit* unit);
	bool run();
	void reset();
};
