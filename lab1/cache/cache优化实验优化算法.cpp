
#include <iostream>
#include <time.h>
using namespace std;

int main()
{
	int n = 0;
	cout << "Please enter the problem size:";
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
	//逐行访问矩阵元素：一步外层循环计算不出任何一个内积，只是向每个内积累加一个乘法结果
	clock_t start_time; // 开始时间
	clock_t end_time;	// 结束时间
	double timesum = 0;
	int i = 0;
	int j = 0;
	int count = 1000;

	while (count)
	{
		start_time = clock();
		for (j = 0; j < n; j++)
		{
			for (i = 0; i < n; i++)
			{
				sum[i] += Arr[j][i] * test[j];
			}
		}
		end_time = clock();
		double time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
		timesum += time;
		count--;
	}
	cout << "Running time:" << timesum;
	return 0;
}
