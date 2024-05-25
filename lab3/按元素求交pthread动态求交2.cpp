
#include<iostream>
#include <string>
#include <fstream>
#include<sstream>
#include<vector>
#include <algorithm> 
#include <pthread.h>
#include <Windows.h>
using namespace std;
#define NUM_THREADS 4 
#pragma comment(lib, "pthreadVC2.lib")
struct thread_param {
	int start;
	int end;
	vector<vector<unsigned int>>* index;
	vector<vector<int>>* query;
	vector<unsigned int>* result;
	int query_index;
};

bool Find(vector<unsigned int>A, unsigned int  a)
{
	for (int i = 0; i < A.size() && A[i] <= a; i++)
	{
		if (A[i] == a)
		{
			return true;
		}
	}
	return false;
}
pthread_mutex_t Mutex;
void* processQuery(void* arg) {
	thread_param* data = (thread_param*)(arg);
	vector<vector<unsigned int>>& index = *(data->index);
	vector<vector<int>>& query = *(data->query);
	vector<unsigned int>& result = *(data->result);
	int query_index = data->query_index;
	int num = query[query_index].size();
	for (int i = data->start; i < data->end; i++)
	{
		bool bo = true;
		unsigned int a = index[query[query_index][0]][i];
		for (int k = 1; k < num; k++) {
			if (!Find(index[query[query_index][k]], a)) {
				bo = false;
				break;
			}
		}
		if (bo) {
			pthread_mutex_lock(&Mutex);
			result.push_back(a);
			pthread_mutex_unlock(&Mutex);
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
	pthread_mutex_init(&Mutex, NULL);
	vector<vector<unsigned int>> results(QueryNum);
	for (int i = 0; i < QueryNum; i++)
	{
		int num = query[i].size();
		vector<unsigned int>result;
		int chunk_size = index[query[i][0]].size() / NUM_THREADS;
		pthread_t threads[NUM_THREADS];
		thread_param params[NUM_THREADS];
		vector<vector<unsigned int>> local_results(NUM_THREADS);
		for (int t = 0; t < NUM_THREADS; t++) {
			params[t].index = &index;
			params[t].query = &query;
			params[t].query_index = i;
			params[t].start = t * chunk_size;
			params[t].result = &local_results[t];
			params[t].end = (t == NUM_THREADS - 1) ? index[query[i][0]].size() : ((t + 1) * chunk_size);
			pthread_create(&threads[t], NULL, processQuery, (void*)&params[t]);
		}

		for (int t = 0; t < NUM_THREADS; t++) {
			pthread_join(threads[t], NULL);
		}
		for (int t = 0; t < NUM_THREADS; t++) {
			result.insert(result.end(), local_results[t].begin(), local_results[t].end());
		}
		results.push_back(result);
	}
	pthread_mutex_destroy(&Mutex);
	// 结束计时
	QueryPerformanceCounter(&end);
	// 计算时间间隔
	interval = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

	std::cout << "代码执行时间: " << interval << " 秒" << std::endl;
	return 0;
}