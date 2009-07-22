#pragma once

class Timer{
public:
	int64_t startTime;
	int64_t stopTime;
	bool running;
	Timer(){
		running=false;
		reset();
	}
	void reset(){
		startTime=0;
		stopTime=0;
		running=false;
	}
	void start(){
		startTime = lucene::util::Misc::currentTimeMillis();
		running=true;
	}
	int32_t split(){
		return lucene::util::Misc::currentTimeMillis()-startTime;
	}
	int32_t stop(){
		if ( running ){
			running=false;
			stopTime = lucene::util::Misc::currentTimeMillis();
		}
		return stopTime-startTime;
	}
	int32_t interval(){
		if (running)
			return lucene::util::Misc::currentTimeMillis()-startTime;
		else
			return stopTime-startTime;
	}

};


typedef int (*PTEST_ROUTINE)(Timer*);
typedef PTEST_ROUTINE LPTEST_ROUTINE;
