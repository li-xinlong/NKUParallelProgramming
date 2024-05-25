#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <bitset>
#include <sys/time.h>
#include <pthread.h>
#include <queue>
#include <arm_neon.h> // Neon头文件
using namespace std;

#define idlength 25205248

int main() {
	cout << "ExpIndex索引规模为1757" << endl;
	cout << "ExpIndex中最大文档ID为25205174,最小文档ID为0" << endl;
	cout << "ExpQuery最大查询规模为1000" << endl;
	int QueryNum=1000;
	// cout << "请输入问题规模(0-1000内的整型)：" << endl;
	// cin >> QueryNum;

	struct timeval start, end, end_f;
	double interval;


	gettimeofday(&start, NULL);

	ifstream readIndex("ExpIndex", ios::binary | ios::in);
	if (!readIndex) {
		cerr << "无法打开ExpIndex文件" << endl;
		return 1;
	}
	vector<vector<unsigned int>> index;
	for (unsigned int i = 0; !readIndex.eof(); ++i) {
		unsigned int n; // 数组的长度
		readIndex.read(reinterpret_cast<char*>(&n), sizeof(n)); // 读取数组长度
		vector<unsigned int> array; // 创建一个容器来存储数组元素
		unsigned int a;
		for (unsigned int j = 0; j < n; j++) {
			readIndex.read(reinterpret_cast<char*>(&a), sizeof(a));
			array.push_back(a);
		}
		if (array.size() != n) {
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
	vector<vector<int>> query;
	string line;
	while (getline(readQuery, line)) {
		vector<int> array; // 创建一个容器来存储数组元素
		stringstream readline(line);
		int a;
		while (readline >> a) {
			array.push_back(a);
		}
		sort(array.begin(), array.end());
		query.push_back(array);
	}
	// 关闭文件流
	readQuery.close();
	gettimeofday(&end_f, NULL);
	interval = (end_f.tv_sec - start.tv_sec) + (end_f.tv_usec - start.tv_usec) / 1000000.0;
	std::cout << "代码执行时间: " << interval << " 秒" << std::endl;
	cout << "读取文件操作完成" << endl;
	vector<vector<unsigned int>> results;
	for (int i = 0; i < QueryNum; i++) {
		int num = query[i].size();
		vector<unsigned int> result;
		bitset<idlength>* bits = new bitset<idlength>[num];
#pragma omp parallel for
		for (int j = 0; j < num; j++)
		{
			for (int k = 0; k < index[query[i][j]].size(); k++)
			{
				bits[j].set(index[query[i][j]][k]);
			}
		}
#pragma omp parallel for
		for (int j = 1; j < num; j++) {
			for (int k = 0; k < idlength / 128; k++) {
		        int* data_second1 = (int*)&bits[0];
				int* add_second1 = (int*)(data_second1 + 4 * k);
				int32x4_t xmm_data_second1 = vld1q_s32(add_second1); // 使用Neon加载数据
				int* data_second2 = (int*)&bits[j];
				int* add_second2 = (int*)(data_second2 + 4 * k);
				int32x4_t xmm_data_second2 = vld1q_s32(add_second2); // 使用Neon加载数据
				xmm_data_second1 = vandq_s32(xmm_data_second1, xmm_data_second2); // 使用Neon进行与操作
				vst1q_s32(add_second1, xmm_data_second1); // 使用Neon存储结果
			}
		}
		int* address = (int*)&bits[0];
#pragma omp parallel for
		for (int j = 0; j < idlength / 128; j++) {
			bitset<128>* temp = (bitset<128>*)(address + j * 4);
			if (*temp != 0) {
				vector<unsigned int> invertedList;
				for (int k = j * 128; k < 128 * (j + 1); k++) {
					if (bits[0][k]) {
						invertedList.push_back(k);
					}
				}
#pragma omp critical
				{
					result.insert(result.end(), invertedList.begin(), invertedList.end());
				}
			}
		}
		results.push_back(result);
	}
	// 结束计时
	gettimeofday(&end, NULL);
	// 计算时间间隔
	interval = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	std::cout << "代码执行时间: " << interval << " 秒" << std::endl;

	return 0;
}
