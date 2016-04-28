#include "../TimerHeap.h"
#include <iostream>

using namespace std;
typedef std::function<void()> TimerCallback;
int main()
{
	auto print1 = [] {cout << "hehe111" << endl; };
	auto print2 = [] {cout << "hehe222" << endl; };
	TimerHeap timer;
	timer.addTimer(int64_t(200), print2);
	timer.addTimer(int64_t(500), print1);
	return 0;
}