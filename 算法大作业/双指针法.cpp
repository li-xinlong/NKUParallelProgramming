#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <windows.h>
using namespace std;

vector<unsigned int> sortedIntersection(const vector<unsigned int>& A, const vector<unsigned int>& B) {
	vector<unsigned int> result;
	auto itA = A.begin(), itB = B.begin();
	while (itA != A.end() && itB != B.end()) {
		if (*itA < *itB) {
			++itA;
		}
		else if (*itB < *itA) {
			++itB;
		}
		else {
			result.push_back(*itA);
			++itA;
			++itB;
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

		vector<unsigned int> result = index[query[i][0]];
		for (size_t j = 1; j < query[i].size(); j++) {
			result = sortedIntersection(result, index[query[i][j]]);
			if (result.empty()) {
				break;
			}
		}
		results.push_back(result);
	}

	QueryPerformanceCounter(&end);
	interval = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
	cout << "代码执行时间: " << interval << " 秒" << endl;
	return 0;
}
