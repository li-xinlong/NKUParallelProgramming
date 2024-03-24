#include <iostream>
#include <Windows.h>
#include<cmath>
using namespace std;

int main()
{
	int n = 0;
	cout << "Please enter the problem size(0-3):";
	int m = 0;
	cin >> m;
	n = pow(8, m);
	int* a = new int[n];
	for (int i = 0; i < n; i++)
	{
		a[i] = i;
	}

	LARGE_INTEGER start_time;
	LARGE_INTEGER end_time;
	double timesum = 0;
	int count = 100000000;

	while (count)
	{
		int sum = 0;//累加结果
		QueryPerformanceCounter(&start_time);
		//优化算法之双路
		int sum1 = 0;
		int sum2 = 0;

		//循环展开，一步循环包含四步操作
		for (int i = 0; i < n; i += 2) {
			sum1 += a[i];
			sum2 += a[i + 1];
			//sum1 += a[i + 2];
			//sum2 += a[i + 3];
			//sum1 += a[i + 4];
			//sum2 += a[i + 5];
			//sum1 += a[i + 6];
			//sum2 += a[i + 7];
			//sum1 += a[i + 8];
		}
		sum = sum1 + sum2;
		QueryPerformanceCounter(&end_time);
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		double time = static_cast<double>(end_time.QuadPart - start_time.QuadPart) / frequency.QuadPart;
		timesum += time;
		count--;
	}
	cout << "Running time:" << timesum;
	return 0;
}
