#ifndef TIME_STAMP_H
#define TIME_STAMP_H

#include "common.h"

class TimeStamp
{
public:
	TimeStamp();
	TimeStamp(int64_t microSecondsSinceEpochArg)
    : microsecondsSinceEpoch(microSecondsSinceEpochArg)
	{
	}
	std::string toString() const;
	int64_t getMicrosecondsSinceEpoch()	{ return microsecondsSinceEpoch; }
	static TimeStamp now();


	static const int kMicroSecondsPerSecond = 1000 * 1000;
private:
	int64_t microsecondsSinceEpoch;
};

inline bool operator<(TimeStamp lts,TimeStamp rts)
{
	return lts.getMicrosecondsSinceEpoch() < rts.getMicrosecondsSinceEpoch();
}

inline bool operator>(TimeStamp lts,TimeStamp rts)
{
	return lts.getMicrosecondsSinceEpoch() > rts.getMicrosecondsSinceEpoch();
}

inline bool operator==(TimeStamp lts,TimeStamp rts)
{
	return lts.getMicrosecondsSinceEpoch() == rts.getMicrosecondsSinceEpoch();
}

#endif // TIME_STAMP_H
