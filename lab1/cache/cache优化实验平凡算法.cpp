#include <iostream>
#include <Windows.h> // 包含Windows平台API头文件

using namespace std;

int main()
{
	int n = 0;
	cout << "Please enter the problem size:";
	cin >> n;
	int** arr = new int* [n];
	for (int i = 0; i < n; i++)
	{
		arr[i] = new int[n];
	}
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			arr[i][j] = i + j;
		}
	}
	int* test = new int[n];
	int* sum = new int[n];
	for (int i = 0; i < n; i++)
	{
		test[i] = i;
		sum[i] = 0;
	}
	LARGE_INTEGER start_time; // 用于存储开始时间
	LARGE_INTEGER end_time;   // 用于存储结束时间
	double timesum = 0;
	int i = 0;
	int j = 0;
	int count = 1000;
	while (count)
	{
		QueryPerformanceCounter(&start_time); // 获取开始时间
		for (i = 0; i < n; i++)
		{
			for (j = 0; j < n; j++)
			{
				sum[i] += arr[j][i] * test[j];
			}
		}
		QueryPerformanceCounter(&end_time); // 获取结束时间
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency); // 获取计时器频率
		double time = static_cast<double>(end_time.QuadPart - start_time.QuadPart) / frequency.QuadPart; // 计算运行时间
		timesum += time;
		count--;
	}

	cout << "Running time:" << timesum;
	return 0;
}
