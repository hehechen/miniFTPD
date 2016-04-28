#include "TimeStamp.h"


TimeStamp::TimeStamp():microsecondsSinceEpoch(0)
{

}


TimeStamp TimeStamp::now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  return TimeStamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}
