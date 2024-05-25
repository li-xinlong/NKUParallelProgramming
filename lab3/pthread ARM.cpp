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
struct resultTask {
	package* packages;
	int start;
	int end;
	int* done; // 表示任务完成的标志
	pthread_mutex_t* mutex; // 用于线程同步的互斥锁
	vector<unsigned int>* result;
};
queue<Task> tasks;
queue<IntersectionTask> intersectionTasks;
queue<resultTask> resultTasks;

pthread_mutex_t taskMutex;
pthread_cond_t taskCond;
pthread_mutex_t intersectionTasksMutex;
pthread_cond_t intersectionTasksCond;
pthread_mutex_t resultTaskMutex;
pthread_cond_t resultTaskCond;
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
				int32x4_t xmm_data_second1 = vld1q_s32(add_second1); // 使用Neon加载数据
				int* data_second2 = (int*)&task->packages[j].first;
				int* add_second2 = (int*)(data_second2 + 4 * k);
				int32x4_t xmm_data_second2 = vld1q_s32(add_second2); // 使用Neon加载数据
				xmm_data_second1 = vandq_s32(xmm_data_second1, xmm_data_second2); // 使用Neon进行与操作
				vst1q_s32(add_second1, xmm_data_second1); // 使用Neon存储结果
			}
		}
		pthread_mutex_lock(task->mutex);
		(*task->done)++;
		pthread_mutex_unlock(task->mutex);
	}
	return NULL;
}

void* resultThreadFunc(void* arg) {
	while (true) {
		pthread_mutex_lock(&resultTaskMutex);
		while (resultTasks.empty()) {
			pthread_cond_wait(&resultTaskCond, &resultTaskMutex);
		}
		resultTask* task = &resultTasks.front();
		resultTasks.pop();
		pthread_mutex_unlock(&resultTaskMutex);

		for (int j = task->start; j < task->end; j++) {
			bitset<128>* temp = (bitset<128>*)((int*)&task->packages->first + j * 4);
			if (*temp != 0) {
				for (int k = j * 128; k < 128 * (j + 1); k++) {
					if (task->packages->first[k]) {
						task->result->push_back(k);
					}
				}
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

	const int numThreads = 4; // Number of threads in the pool
	pthread_t threads[numThreads];
	pthread_mutex_init(&taskMutex, NULL);
	pthread_cond_init(&taskCond, NULL);
	pthread_mutex_init(&intersectionTasksMutex, NULL);
	pthread_cond_init(&intersectionTasksCond, NULL);
	pthread_mutex_init(&resultTaskMutex, NULL);
	pthread_cond_init(&resultTaskCond, NULL);

	for (int i = 0; i < numThreads; i++) {
		pthread_create(&threads[i], NULL, threadFunc, NULL);
	}
	const int numIntersectionThreads = 4; // Number of threads for intersection
	pthread_t intersectionThreads[numIntersectionThreads];
	for (int i = 0; i < numIntersectionThreads; i++) {
		pthread_create(&intersectionThreads[i], NULL, intersectionThreadFunc, NULL);
	}
	pthread_t resultThreads[numIntersectionThreads];
	for (int i = 0; i < numIntersectionThreads; i++) {
		pthread_create(&resultThreads[i], NULL, resultThreadFunc, NULL);
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
		done = 0;
		vector<unsigned int> local_results[numIntersectionThreads];
		for (int t = 0; t < numIntersectionThreads; t++) {
			resultTask resulttask;
			resulttask.start = t * chunk_size;
			resulttask.end = (t == numIntersectionThreads - 1) ? (idlength / 128) : ((t + 1) * chunk_size);
			resulttask.mutex = &doneMutex;
			resulttask.done = &done;
			resulttask.packages = &packages[0];
			resulttask.result = &local_results[t];
			pthread_mutex_lock(&resultTaskMutex);
			resultTasks.push(resulttask);
			pthread_mutex_unlock(&resultTaskMutex);
			pthread_cond_signal(&resultTaskCond);
		}
		while (true) {
			pthread_mutex_lock(&doneMutex);
			if (done == numIntersectionThreads) {
				pthread_mutex_unlock(&doneMutex);
				break;
			}
			pthread_mutex_unlock(&doneMutex);
		}
		for (int t = 0; t < numIntersectionThreads; t++) {
			result.insert(result.end(), local_results[t].begin(), local_results[t].end());
		}
		results.push_back(result);
		pthread_mutex_destroy(&doneMutex);
		delete[] packages;
	}

	// 结束计时
	gettimeofday(&end, NULL);
	// 计算时间间隔
	interval = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
	std::cout << "代码执行时间: " << interval << " 秒" << std::endl;
	pthread_mutex_destroy(&taskMutex);
	pthread_cond_destroy(&taskCond);
	pthread_mutex_destroy(&intersectionTasksMutex);
	pthread_cond_destroy(&intersectionTasksCond);
	pthread_mutex_destroy(&resultTaskMutex);
	pthread_cond_destroy(&resultTaskCond);

	return 0;
}
