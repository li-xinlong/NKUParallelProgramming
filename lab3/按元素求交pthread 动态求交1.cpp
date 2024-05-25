#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <pthread.h>
#include <Windows.h>

using namespace std;

struct ThreadData {
	vector<vector<unsigned int>>* index;
	vector<vector<int>>* query;
	vector<vector<unsigned int>>* results;
	int query_index;
};

bool Find(vector<unsigned int>& A, unsigned int a) {
	for (size_t i = 0; i < A.size() && A[i] <= a; i++) {
		if (A[i] == a) {
			return true;
		}
	}
	return false;
}

void* processQuery(void* arg) {
	ThreadData* data = reinterpret_cast<ThreadData*>(arg);
	int query_index = data->query_index;
	vector<vector<unsigned int>>& index = *(data->index);
	vector<vector<int>>& query = *(data->query);
	vector<vector<unsigned int>>& results = *(data->results);

	int num = query[query_index].size();
	vector<unsigned int> result;
	for (size_t j = 0; j < index[query[query_index][0]].size(); j++) {
		bool bo = true;
		unsigned int a = index[query[query_index][0]][j];
		for (int k = 1; k < num; k++) {
			if (!Find(index[query[query_index][k]], a)) {
				bo = false;
				break;
			}
		}
		if (bo) {
			result.push_back(a);
		}
	}

	results[query_index] = result;

	pthread_exit(NULL);
}

int main() {
	cout << "ExpIndex索引规模为1757" << endl;
	cout << "ExpIndex中最大文档ID为25205174,最小文档ID为0" << endl;
	cout << "ExpQuery最大查询规模为1000" << endl;
	int QueryNum;
	cout << "请输入问题规模(0-1000内的整型)：" << endl;

	cin >> QueryNum;
	LARGE_INTEGER frequency;
	LARGE_INTEGER start, end;

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
		sort(array.begin(), array.end());
		query.push_back(array);
	}
	readQuery.close();

	// 结束计时
	QueryPerformanceCounter(&end);
	double readTime = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
	cout << "读取文件时间: " << readTime << " 秒" << endl;
	cout << "读取文件操作完成" << endl;

	// 初始化结果向量
	vector<vector<unsigned int>> results(QueryNum);

	// 创建线程和线程数据
	pthread_t threads[QueryNum];
	ThreadData thread_data[QueryNum];
	for (int i = 0; i < QueryNum; ++i) {
		thread_data[i].index = &index;
		thread_data[i].query = &query;
		thread_data[i].results = &results;
		thread_data[i].query_index = i;
		pthread_create(&threads[i], nullptr, processQuery, &thread_data[i]);
	}

	// 等待所有线程完成
	for (int i = 0; i < QueryNum; ++i) {
		pthread_join(threads[i], nullptr);
	}

	// 计算执行时间
	QueryPerformanceCounter(&end);
	double execTime = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
	cout << "代码执行时间: " << execTime << " 秒" << endl;

	return 0;
}
