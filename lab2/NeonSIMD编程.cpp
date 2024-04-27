#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm> 
#include <bitset>
#include <arm_neon.h> // 包含 NEON 头文件
#include <sys/time.h>   // For gettimeofday()
#define idlength 25205248
using namespace std;
int main()
{
	cout << "ExpIndex索引规模为1757" << endl;
	cout << "ExpIndex中最大文档ID为25205174,最小文档ID为0" << endl;
	cout << "ExpQuery最大查询规模为1000" << endl;
	int QueryNum;
	cout << "请输入问题规模(0-1000内的整型)：" << endl;

	//  cin >> QueryNum;
	QueryNum=1000;
    struct timeval start ,end1,end2;
	double interval;
	gettimeofday(&start,NULL);
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
	gettimeofday(&end1,NULL);
	interval = (end1.tv_sec - start.tv_sec) + (end1.tv_usec - start.tv_usec) / 1000000.0;

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
		for (int j = 1; j < num; j++)
		{
			for (int k = 0; k < idlength / 128; k++)
			{
				int* data1 = (int*)&bits[0];
				int* add1 = (int*)(data1 + 4 * k);
                int32x4_t add1_neon=vld1q_s32(add1);
				int* data2 = (int*)&bits[j];
				int* add2 = (int*)(data2 + 4 * k);
				int32x4_t add2_neon=vld1q_s32(add2);
				add1_neon=vandq_s32(add1_neon,add2_neon);
				 vst1q_s32(add1,add1_neon);
			}
		}
		int* address = (int*)&bits[0];
		for (int j = 0; j < idlength / 128; j++)
		{
			bitset<128>* temp = (bitset<128>*)(address + j * 4);
			if (*temp != 0)
			{
				for (int k = j * 128; k < 128 * (j + 1); k++)
				{
					if (bits[0][k])
					{
						result.push_back(k);
					}
				}
			}
		}
		results.push_back(result);
	}
	//结束计时
	gettimeofday(&end2,NULL);
	// 计算时间间隔
	interval = (end2.tv_sec - start.tv_sec) + (end2.tv_usec - start.tv_usec) / 1000000.0;
	std::cout << "代码执行时间: " << interval << " 秒" << std::endl;
	/*for (int i = 0; i < results.size(); i++) {
		for (int j = 0; j < results[i].size(); j++) {
			cout << results[i][j] << " ";
		}
		cout << endl;
	}*/
	return 0;
}