#pragma once

int BenchmarkDocumentWriter(Timer*);
int BenchmarkTermDocs(Timer* timerCase);

class TestCLString:public Unit
{
protected:
	void runTests(){
		this->runTest("BenchmarkDocumentWriter",BenchmarkDocumentWriter,10);
		//this->runTest("BenchmarkTermDocs",BenchmarkTermDocs,100);
	}
public:
	const char* getName(){
		return "TestCLString";
	}
};
