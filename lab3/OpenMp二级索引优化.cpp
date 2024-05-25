#include <iostream>
#include <string>
#include <fstream>
#include<sstream>
#include<vector>
#include <algorithm> 
#include <bitset>
#include <Windows.h>
#include <emmintrin.h>  //SSE2

#pragma comment(lib, "pthreadVC2.lib")
using namespace std;
#define idlength 25214976
struct package {
	bitset<idlength> first; //一级索引
	bitset<idlength / 128>second;
	vector<unsigned int>index;
};

void* tobits(void* parm)
{
	package* pack = (package*)parm;
	for (int i = 0; i < pack->index.size(); i++)
	{
		pack->first.set(pack->index[i]);//转换为一级索引
	}
	long* temp = (long*)&pack->first;
	for (int i = 0; i < idlength / 128; i++)
	{
		bitset<128>* bitset_temp = (bitset<128>*)(temp + i * 2);
		if (*bitset_temp == 1)
		{
			pack->second.set(i);
		}
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
#pragma omp parallel for
		for (int j = 0; j < num; j++)
		{
			packages[j].index = index[query[i][j]];
		}
#pragma omp parallel for
		for (int j = 0; j < num; j++) {
			tobits(&packages[j]);
		}
		for (int j = 1; j < num; j++)
		{
			for (int k = 0; k < idlength / 128 / 128; k++)
			{
				int* data_second1 = (int*)&packages[0].second;//每个整型32位，128位8个整型
				// 使用 sse2 加载数据到寄存器
				int* add_second1 = (int*)(data_second1 + 4 * k);
				__m128i xmm_data_second1 = _mm_loadu_si128((__m128i*)add_second1);
				// 定义一个数组用于存储第二个位集的数据
				int* data_second2 = (int*)&packages[j].second;//每个整型32位，128位8个整型
				int* add_second2 = (int*)(data_second2 + 4 * k);
				// 使用 sse2 加载数据到第二个寄存器
				__m128i xmm_data_second2 = _mm_loadu_si128((__m128i*)add_second2);
				xmm_data_second1 = _mm_and_si128(xmm_data_second1, xmm_data_second2);
				_mm_storeu_si128((__m128i*)add_second1, xmm_data_second1);
				for (int m = k * 128; m < 128 * (k + 1); m++) {
					if (packages[j].second[m])
					{
						int* data1 = (int*)&packages[0].first;//每个整型32位，128位8个整型
						// 使用 sse2 加载数据到寄存器
						int* add1 = (int*)(data1 + 4 * m);
						__m128i xmm_data1 = _mm_loadu_si128((__m128i*)add1);
						// 定义一个数组用于存储第二个位集的数据
						int* data2 = (int*)&packages[j].first;//每个整型32位，128位8个整型
						int* add2 = (int*)(data2 + 4 * m);
						// 使用 sse2 加载数据到第二个寄存器
						__m128i xmm_data2 = _mm_loadu_si128((__m128i*)add2);
						xmm_data1 = _mm_and_si128(xmm_data1, xmm_data2);
						_mm_storeu_si128((__m128i*)add1, xmm_data1);
					}
					else
					{
						int* data1 = (int*)&packages[0].first;//每个整型32位，128位8个整型
						int* add1 = (int*)(data1 + 4 * m);
						__m128i zero_vector = _mm_setzero_si128();
						_mm_storeu_si128((__m128i*)add1, zero_vector);
					}

				}
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