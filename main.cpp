#include<iostream>
#include "parseconfig.h"
#include "sysutil.h"
#include "FtpdController.h"
#include "TimerHeap.h"
using namespace std;
int main()
{
	cout<<"hehe"<<endl;
	ParseConfig *parseconfig = ParseConfig::getInstance();
	parseconfig->loadfile();
	FtpdController ftpd;
	ftpd.loop();
	return 0;
}

// int main()
// {
// 	auto print1 = [] {cout << "hehe111" << endl; };
// 	auto print2 = [] {cout << "hehe222" << endl; };
// 	TimerHeap timer;
// 	timer.addTimer(int64_t(1000000), print2);
// 	timer.addTimer(int64_t(50000), print1);
// 	while(1);
// 	return 0;
// }