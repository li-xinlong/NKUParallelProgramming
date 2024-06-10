#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <windows.h>

using namespace std;

vector<unsigned int> divideAndConquerIntersection(const vector<vector<unsigned int>>& arrays, int start, int end) {
	if (start == end) {
		return arrays[start];
	}
	int mid = (start + end) / 2;
	vector<unsigned int> left = divideAndConquerIntersection(arrays, start, mid);
	vector<unsigned int> right = divideAndConquerIntersection(arrays, mid + 1, end);
	vector<unsigned int> result;
	size_t i = 0, j = 0;
	while (i < left.size() && j < right.size()) {
		if (left[i] < right[j]) {
			++i;
		}
		else if (left[i] > right[j]) {
			++j;
		}
		else {
			result.push_back(left[i]);
			++i;
			++j;
		}
	}
	return result;
}

int main() {
	cout << "ExpIndex索引规模为1757" << endl;
	cout << "ExpIndex中最大文档ID为25205174,最小文档ID为0" << endl;
	cout << "ExpQuery最大查询规模为1000" << endl;
	int QueryNum;
	cout << "请输入问题规模(0-1000内的整型)：" << endl;
	cin >> QueryNum;

	LARGE_INTEGER frequency;
	LARGE_INTEGER start, end, end_f;
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

	vector<vector<unsigned int>> index;
	unsigned int n;
	while (readIndex.read(reinterpret_cast<char*>(&n), sizeof(n))) {
		vector<unsigned int> array(n);
		if (!readIndex.read(reinterpret_cast<char*>(array.data()), n * sizeof(unsigned int))) {
			cerr << "读取ExpIndex数组失败" << endl;
		}
		sort(array.begin(), array.end());
		index.push_back(array);
	}
	readIndex.close();

	ifstream readQuery("ExpQuery", ios::in);
	if (!readQuery) {
		cerr << "无法打开ExpQuery文件" << endl;
		return 1;
	}

	vector<vector<int>> query;
	string line;
	while (getline(readQuery, line)) {
		vector<int> array;
		stringstream readline(line);
		int a;
		while (readline >> a) {
			array.push_back(a);
		}
		query.push_back(array);
	}
	readQuery.close();

	QueryPerformanceCounter(&end_f);
	interval = static_cast<double>(end_f.QuadPart - start.QuadPart) / frequency.QuadPart;
	cout << "读取文件时间: " << interval << " 秒" << endl;
	cout << "读取文件操作完成" << endl;

	vector<vector<unsigned int>> results;
	for (int i = 0; i < QueryNum; i++) {
		if (query[i].empty()) {
			results.push_back({});
			continue;
		}

		vector<vector<unsigned int>> subsets;
		for (size_t j = 0; j < query[i].size(); j++) {
			subsets.push_back(index[query[i][j]]);
		}
		vector<unsigned int> result = divideAndConquerIntersection(subsets, 0, subsets.size() - 1);
		results.push_back(result);
	}

	QueryPerformanceCounter(&end);
	interval = static_cast<double>(end.QuadPart - end_f.QuadPart) / frequency.QuadPart;
	cout << "代码执行时间: " << interval << " 秒" << endl;
	for (int i = 0; i < results.size(); i++)
	{
		cout << results[i].size() << endl;
	}
	return 0;
}
