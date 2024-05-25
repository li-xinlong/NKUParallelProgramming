#include <iostream>
#include <string>
#include <fstream>
#include<sstream>
#include<vector>
#include <algorithm> 
#include <bitset>
#include <Windows.h>
#include <emmintrin.h>  //SSE2
#include <pthread.h>

#pragma comment(lib, "pthreadVC2.lib")
using namespace std;
#define idlength 25205248
struct package {
	bitset<idlength> first; //一级索引
	vector<unsigned int>index;
};
void* tobits(void* parm)
{
	package* pack = (package*)parm;
	for (int i = 0; i < pack->index.size(); i++)
	{
		pack->first.set(pack->index[i]);//转换为一级索引
	}
	return NULL;
}
int main()
{
	cout << "ExpIndex索引规模为1757" << endl;
	cout << "ExpIndex中最大文档ID为25205174,最小文档ID为0" << endl;
	cout << "ExpQuery最大查询规模为1000" << endl;
	int QueryNum;
	cout << "请输入问题规模(0-1000内的整型)：" << endl;

	cin >> QueryNum;
	LARGE_INTEGER frequency;

	LARGE_INTEGER start;

	LARGE_INTEGER end;
	LARGE_INTEGER end_f;
	double interval;
	// 获取性能计数器频率
	QueryPerformanceFrequency(&frequency);
	// 开始计时
	QueryPerformanceCounter(&start);
	ifstream readIndex("ExpIndex", ios::binary | ios::in);
	if (!readIndex) {
		cerr << "无法打开ExpIndex文件" << endl;
		return 1;
	}
	vector<vector<unsigned int>>index;
	for (unsigned int i = 0; !readIndex.eof(); ++i) {
		unsigned int n; // 数组的长度
		readIndex.read(reinterpret_cast<char*>(&n), sizeof(n)); // 读取数组长度
		vector<unsigned int> array; // 创建一个容器来存储数组元素
		unsigned int a;
		for (unsigned int j = 0; j < n; j++)
		{
			readIndex.read(reinterpret_cast<char*>(&a), sizeof(a));
			array.push_back(a);
		}
		if (array.size() != n)
		{
			cerr << "读取ExpIndex第" << i << "数组失败" << endl;
		}
		sort(array.begin(), array.end());
		index.push_back(array);
	}
	// 关闭文件流
	readIndex.close();
	ifstream readQuery("ExpQuery", ios::in);
	if (!readQuery) {
		cerr << "无法打开ExpQuery文件" << endl;
		return 1;
	}
	vector<vector<int>>query;
	string line;
	while (getline(readQuery, line))
	{
		vector<int> array; // 创建一个容器来存储数组元素
		stringstream readline(line);
		int a;
		while (readline >> a)
		{
			array.push_back(a);
		}
		sort(array.begin(), array.end());
		query.push_back(array);
	}
	// 关闭文件流
	readQuery.close();
	QueryPerformanceCounter(&end_f);
	interval = static_cast<double>(end_f.QuadPart - start.QuadPart) / frequency.QuadPart;

	std::cout << "读取文件时间: " << interval << " 秒" << std::endl;
	cout << "读取文件操作完成" << endl;
	vector<vector<unsigned int>>results;
	for (int i = 0; i < QueryNum; i++)
	{
		int num = query[i].size();
		vector<unsigned int>result;
		package* packages = new package[num];
		for (int j = 0; j < num; j++)
		{
			packages[j].index = index[query[i][j]];
		}
		pthread_t* threads = new pthread_t[num];
		for (int j = 0; j < num; j++)
		{
			pthread_create(&threads[j], NULL, tobits, (void*)&packages[j]);
		}
		for (int j = 0; j < num; j++)
		{
			pthread_join(threads[j], NULL);
		}
		for (int j = 1; j < num; j++)
		{
			for (int k = 0; k < idlength / 128; k++)
			{
				int* data1 = (int*)&packages[0].first;
				int* add1 = (int*)(data1 + 4 * k);
				__m128i xmm_data1 = _mm_loadu_si128((__m128i*)add1);
				int* data2 = (int*)&packages[j].first;
				int* add2 = (int*)(data2 + 4 * k);
				__m128i xmm_data2 = _mm_loadu_si128((__m128i*)add2);
				xmm_data1 = _mm_and_si128(xmm_data1, xmm_data2);
				_mm_storeu_si128((__m128i*)add1, xmm_data1);
			}

		}
		int* address = (int*)&packages[0].first;
		for (int j = 0; j < idlength / 128; j++)
		{
			bitset<128>* temp = (bitset<128>*)(address + j * 4);
			if (*temp != 0)
			{
				for (int k = j * 128; k < 128 * (j + 1); k++)
				{
					if (packages[0].first[k])
					{
						result.push_back(k);
					}
				}
			}
		}
		results.push_back(result);
	}
	//结束计时
	QueryPerformanceCounter(&end);
	// 计算时间间隔
	interval = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
	std::cout << "代码执行时间: " << interval << " 秒" << std::endl;
	return 0;
}