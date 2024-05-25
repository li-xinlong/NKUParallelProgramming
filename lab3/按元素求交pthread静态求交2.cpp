#include<queue>
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
struct Task {
	thread_param* data;
	pthread_mutex_t* mutex;
	int* done;
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
queue<Task> tasks;
pthread_mutex_t Mutex;
pthread_mutex_t taskMutex;
pthread_cond_t taskCond;
void* processQuery(void* arg) {
	while (true) {
		pthread_mutex_lock(&taskMutex);
		while (tasks.empty()) {
			pthread_cond_wait(&taskCond, &taskMutex);
		}
		Task* task = &tasks.front();
		tasks.pop();
		pthread_mutex_unlock(&taskMutex);
		thread_param* data = task->data;
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
		pthread_mutex_lock(task->mutex);
		(*task->done)++;
		pthread_mutex_unlock(task->mutex);
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
	pthread_mutex_init(&taskMutex, NULL);
	pthread_cond_init(&taskCond, NULL);
	vector<vector<unsigned int>> results(QueryNum);

	pthread_t threads[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++)
	{
		pthread_create(&threads[i], NULL, processQuery, NULL);
	}
	for (int i = 0; i < QueryNum; i++)
	{
		int num = query[i].size();
		vector<unsigned int>result;
		int chunk_size = index[query[i][0]].size() / NUM_THREADS;

		thread_param param;
		param.index = &index;
		param.query = &query;
		Task task;
		pthread_mutex_t doneMutex;
		pthread_mutex_init(&doneMutex, NULL);
		vector<vector<unsigned int>> local_results(NUM_THREADS);
		int done = 0;
		task.done = &done;
		task.mutex = &doneMutex;

		for (int t = 0; t < NUM_THREADS; t++) {
			param.query_index = i;
			param.start = t * chunk_size;
			param.result = &local_results[t];
			param.end = (t == NUM_THREADS - 1) ? index[query[i][0]].size() : ((t + 1) * chunk_size);
			task.data = &param;
			pthread_mutex_lock(&taskMutex);
			tasks.push(task);
			pthread_mutex_unlock(&taskMutex);
			pthread_cond_signal(&taskCond);
		}

		while (true) {
			pthread_mutex_lock(&doneMutex);
			if (done == NUM_THREADS) {
				pthread_mutex_unlock(&doneMutex);
				break;
			}
			pthread_mutex_unlock(&doneMutex);
		}
		for (int t = 0; t < NUM_THREADS; t++) {
			result.insert(result.end(), local_results[t].begin(), local_results[t].end());
		}
		results.push_back(result);
		pthread_mutex_destroy(&doneMutex);
	}
	// 结束计时
	QueryPerformanceCounter(&end);
	// 计算时间间隔
	interval = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

	std::cout << "代码执行时间: " << interval << " 秒" << std::endl;
	pthread_mutex_destroy(&Mutex);
	pthread_mutex_destroy(&taskMutex);
	pthread_cond_destroy(&taskCond);
	return 0;
}