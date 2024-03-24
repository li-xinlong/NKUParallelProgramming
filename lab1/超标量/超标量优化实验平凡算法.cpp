#include <iostream>
#include <Windows.h>
#include<cmath>
using namespace std;

int main()
{
	int n = 0;
	cout << "Please enter the problem size(0-3)";
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
		int sum = 0;
		QueryPerformanceCounter(&start_time);
		for (int i = 0; i < n; i++)
		{
			sum += a[i];
		}
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
