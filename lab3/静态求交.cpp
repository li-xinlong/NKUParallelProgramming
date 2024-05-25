#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <bitset>
#include <Windows.h>
#include <emmintrin.h>  //SSE2
#include <pthread.h>
#include <queue>

#pragma comment(lib, "pthreadVC2.lib")

using namespace std;
#define idlength 25205248

struct package {
	bitset<idlength> first; //一级索引
	vector<unsigned int> index;
};

struct Task {
	package* pack;
	pthread_mutex_t* mutex;
	int* done;
};
struct IntersectionTask {
	package* packages;
	int start;
	int end;
	int num;
	int* done; // 表示任务完成的标志
	pthread_mutex_t* mutex; // 用于线程同步的互斥锁
};
queue<Task> tasks;
queue<IntersectionTask> intersectionTasks;
pthread_mutex_t taskMutex;
pthread_cond_t taskCond;
pthread_mutex_t intersectionTasksMutex;
pthread_cond_t intersectionTasksCond;
void* threadFunc(void* arg) {
	while (true) {
		pthread_mutex_lock(&taskMutex);
		while (tasks.empty()) {
			pthread_cond_wait(&taskCond, &taskMutex);
		}
		Task task = tasks.front();
		tasks.pop();
		pthread_mutex_unlock(&taskMutex);

		package* pack = task.pack;
		for (unsigned int i = 0; i < pack->index.size(); i++) {
			pack->first.set(pack->index[i]); //转换为一级索引
		}

		pthread_mutex_lock(task.mutex);
		(*task.done)++;
		pthread_mutex_unlock(task.mutex);
	}
	return NULL;
}
void* intersectionThreadFunc(void* arg) {
	while (true) {
		pthread_mutex_lock(&intersectionTasksMutex);
		while (intersectionTasks.empty()) {
			pthread_cond_wait(&intersectionTasksCond, &intersectionTasksMutex);
		}
		IntersectionTask* task = &intersectionTasks.front();
		intersectionTasks.pop();
		pthread_mutex_unlock(&intersectionTasksMutex);

		for (int j = 1; j < task->num; j++) {
			for (int k = task->start; k < task->end; k++) {
				int* data_second1 = (int*)&task->packages[0].first;
				int* add_second1 = (int*)(data_second1 + 4 * k);
				__m128i xmm_data_second1 = _mm_loadu_si128((__m128i*)add_second1);
				int* data_second2 = (int*)&task->packages[j].first;
				int* add_second2 = (int*)(data_second2 + 4 * k);
				__m128i xmm_data_second2 = _mm_loadu_si128((__m128i*)add_second2);
				xmm_data_second1 = _mm_and_si128(xmm_data_second1, xmm_data_second2);
				_mm_storeu_si128((__m128i*)add_second1, xmm_data_second1);
			}
		}
		pthread_mutex_lock(task->mutex);
		(*task->done)++;
		pthread_mutex_unlock(task->mutex);
	}
	return NULL;
}
int main() {
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

	QueryPerformanceCounter(&end_f);
	interval = static_cast<double>(end_f.QuadPart - start.QuadPart) / frequency.QuadPart;
	std::cout << "读取文件时间: " << interval << " 秒" << std::endl;
	cout << "读取文件操作完成" << endl;

	vector<vector<unsigned int>> results;

	const int numThreads = 4; // Number of threads in the pool
	pthread_t threads[numThreads];
	pthread_mutex_init(&taskMutex, NULL);
	pthread_cond_init(&taskCond, NULL);
	pthread_mutex_init(&intersectionTasksMutex, NULL);
	pthread_cond_init(&intersectionTasksCond, NULL);

	for (int i = 0; i < numThreads; i++) {
		pthread_create(&threads[i], NULL, threadFunc, NULL);
	}
	const int numIntersectionThreads = 4; // Number of threads for intersection
	pthread_t intersectionThreads[numIntersectionThreads];
	for (int i = 0; i < numIntersectionThreads; i++) {
		pthread_create(&intersectionThreads[i], NULL, intersectionThreadFunc, NULL);
	}

	for (int i = 0; i < QueryNum; i++) {
		int num = query[i].size();
		vector<unsigned int> result;
		package* packages = new package[num];
		int done = 0;
		pthread_mutex_t doneMutex;
		pthread_mutex_init(&doneMutex, NULL);

		for (int j = 0; j < num; j++) {
			packages[j].index = index[query[i][j]];
			Task task = { &packages[j], &doneMutex, &done };
			pthread_mutex_lock(&taskMutex);
			tasks.push(task);
			pthread_mutex_unlock(&taskMutex);
			pthread_cond_signal(&taskCond);
		}

		while (true) {
			pthread_mutex_lock(&doneMutex);
			if (done == num) {
				pthread_mutex_unlock(&doneMutex);
				break;
			}
			pthread_mutex_unlock(&doneMutex);
		}
		done = 0;
		int chunk_size = idlength / 128 / numIntersectionThreads;
		for (int t = 0; t < numIntersectionThreads; t++)
		{
			IntersectionTask intersectionTask;
			intersectionTask.packages = packages;
			intersectionTask.done = &done;
			intersectionTask.mutex = &doneMutex;
			intersectionTask.num = num;
			intersectionTask.start = t * chunk_size;
			intersectionTask.end = (t == numIntersectionThreads - 1) ? (idlength / 128) : ((t + 1) * chunk_size);
			pthread_mutex_lock(&intersectionTasksMutex);
			intersectionTasks.push(intersectionTask);
			pthread_mutex_unlock(&intersectionTasksMutex);
			pthread_cond_signal(&intersectionTasksCond);
		}
		while (true) {
			pthread_mutex_lock(&doneMutex);
			if (done == numIntersectionThreads) {
				pthread_mutex_unlock(&doneMutex);
				break;
			}
			pthread_mutex_unlock(&doneMutex);
		}
		int* address = (int*)&packages[0].first;
		for (int j = 0; j < idlength / 128; j++) {
			bitset<128>* temp = (bitset<128>*)(address + j * 4);
			if (*temp != 0) {
				for (int k = j * 128; k < 128 * (j + 1); k++) {
					if (packages[0].first[k]) {
						result.push_back(k);
					}
				}
			}
		}
		results.push_back(result);
		pthread_mutex_destroy(&doneMutex);
		delete[] packages;
	}

	// 结束计时
	QueryPerformanceCounter(&end);
	// 计算时间间隔
	interval = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
	std::cout << "代码执行时间: " << interval << " 秒" << std::endl;
	cout << static_cast<double>(end.QuadPart - end_f.QuadPart) / frequency.QuadPart;
	pthread_mutex_destroy(&taskMutex);
	pthread_cond_destroy(&taskCond);
	pthread_mutex_destroy(&intersectionTasksMutex);
	pthread_cond_destroy(&intersectionTasksCond);

	return 0;
}
