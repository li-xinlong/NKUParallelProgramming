#include <iostream>
#include <string>
#include <fstream>
#include<sstream>
#include<vector>
#include <algorithm> 
#include <bitset>
#include <Windows.h>
#include <emmintrin.h>  //SSE2
#define idlength 25205248
using namespace std;
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
	unsigned int n; // 数组的长度
	int i = 0;
	while (readIndex.read(reinterpret_cast<char*>(&n), sizeof(n))) {
		vector<unsigned int> array(n); // 创建一个容器来存储数组元素
		if (!readIndex.read(reinterpret_cast<char*>(array.data()), n * sizeof(unsigned int)))
		{
			cerr << "读取ExpIndex第" << i << "数组失败" << endl;
		}
		sort(array.begin(), array.end());
		index.push_back(array);
		i++;
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

	//cout << "索引表的总长度" << index.size() << endl;
	vector<vector<unsigned int>>results;
	for (int i = 0; i < QueryNum; i++)
	{
		int num = query[i].size();
		vector<unsigned int>result;
		bitset<idlength>* bits = new bitset<idlength>[num];
		for (int j = 0; j < num; j++)
		{
			for (int k = 0; k < index[query[i][j]].size(); k++)
			{
				bits[j].set(index[query[i][j]][k]);
			}
		}
		/*for (int j = 1; j < num; j++)
		{
			bits[0] &= bits[j];
		}*/
		for (int j = 1; j < num; j++)
		{
			for (int k = 0; k < idlength; k++)
			{
				bits[0][k] = bits[0][k] * bits[j][k];
			}
		}
		for (int j = 0; j < idlength; j++)
		{
			if (bits[0][j] == 1)
			{
				result.push_back(j);
			}
		}
		results.push_back(result);
	}
	//结束计时
	QueryPerformanceCounter(&end);
	// 计算时间间隔
	interval = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
	std::cout << "代码执行时间: " << interval << " 秒" << std::endl;
	/*for (int i = 0; i < results.size(); i++) {
		for (int j = 0; j < results[i].size(); j++) {
			cout << results[i][j] << " ";
		}
		cout << endl;
	}*/
	return 0;
}