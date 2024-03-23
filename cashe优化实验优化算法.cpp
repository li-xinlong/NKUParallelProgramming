
#include <iostream>
#include<windows.h>
using namespace std;

int main()
{
	int n = 0;
	cout << "请输入问题规模：";
	cin >> n;
	int** Arr = new int* [n];
	for (int i = 0; i < n; i++)
	{
		Arr[i] = new int[n];
	}
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			Arr[i][j] = i + j;
		}
	}
	int* test = new int[n];
	int* sum = new int[n];
	for (int i = 0; i < n; i++)
	{
		test[i] = i;
		sum[i] = 0;
	}
	//逐列访问矩阵元素：一步外层循环（内存循环一次完整执行）计算出一个内积结果
	LARGE_INTEGER frequency; // 用于获取计时器的频率
	LARGE_INTEGER start_time; // 用于存储开始时间
	LARGE_INTEGER end_time;   // 用于存储结束时间
	double timesum = 0;
	for (int k = 0; k < 10; k++)
	{
		int count = 1000;
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&start_time);
		while (count)
		{
			for (int j = 0; j < n; j++)
			{
				for (int i = 0; i < n; i++)
				{
					sum[i] += Arr[j][i] * test[j];
				}
			}
			count--;
		}
		QueryPerformanceCounter(&end_time);
		double time = static_cast<double>(end_time.QuadPart - start_time.QuadPart) / frequency.QuadPart;
		timesum += time;
	}
	cout << "运行时间：" << timesum / 10;
	return 0;
}

