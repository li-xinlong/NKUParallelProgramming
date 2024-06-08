#include <mpi.h>
#include<iostream>
#include <string>
#include <fstream>
#include<sstream>
#include<vector>
#include <algorithm> 
#include <Windows.h>
using namespace std;
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
int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);

	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	int QueryNum = 48;
	int world_rank;
	int part = QueryNum / world_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	LARGE_INTEGER frequency;

	LARGE_INTEGER start;
	LARGE_INTEGER end;
	LARGE_INTEGER end_f;
	double interval;
	// 获取性能计数器频率
	vector<vector<unsigned int>> index;
	vector<vector<int>> query;
	QueryPerformanceFrequency(&frequency);
	// 开始计时
	QueryPerformanceCounter(&start);
	if (world_rank == 0) {
		ifstream readIndex("ExpIndex", ios::binary | ios::in);
		if (!readIndex) {
			cerr << "无法打开ExpIndex文件" << endl;
			MPI_Abort(MPI_COMM_WORLD, 1);
			MPI_Finalize();
			return 1;
		}

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
			MPI_Abort(MPI_COMM_WORLD, 1);
			MPI_Finalize();
			return 1;
		}

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
		if (world_rank == 0) {
			cout << "ExpIndex索引规模为1757" << endl;
			cout << "ExpIndex中最大文档ID为25205174,最小文档ID为0" << endl;
			cout << "ExpQuery最大查询规模为1000" << endl;
			QueryPerformanceCounter(&end_f);
			interval = static_cast<double>(end_f.QuadPart - start.QuadPart) / frequency.QuadPart;
			cout << "读取文件时间: " << interval << " 秒" << endl;
			cout << "读取文件操作完成" << endl;
		}
	}

	int index_size = index.size();
	MPI_Bcast(&index_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if (world_rank != 0) {
		index.resize(index_size);
	}
	for (int i = 0; i < index_size; ++i) {
		int array_size = (world_rank == 0) ? index[i].size() : 0;
		MPI_Bcast(&array_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
		if (world_rank != 0) {
			index[i].resize(array_size);
		}
		MPI_Bcast(index[i].data(), array_size, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
	}

	int query_size = query.size();
	MPI_Bcast(&query_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if (world_rank != 0) {
		query.resize(query_size);
	}
	for (int i = 0; i < query_size; ++i) {
		int array_size = (world_rank == 0) ? query[i].size() : 0;
		MPI_Bcast(&array_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
		if (world_rank != 0) {
			query[i].resize(array_size);
		}
		MPI_Bcast(query[i].data(), array_size, MPI_INT, 0, MPI_COMM_WORLD);
	}
	vector<vector<unsigned int>>local_results;
	int begin = world_rank;
	for (int i = begin; i < QueryNum; i += world_size)
	{
		int num = query[i].size();
		vector<unsigned int>result;
		for (int j = 0; j < index[query[i][0]].size(); j++)
		{
			bool bo = true;
			unsigned int a = index[query[i][0]][j];
			for (int k = 1; k < num; k++)
			{
				if (!Find(index[query[i][k]], a))
				{
					bo = false;
					break;
				}
			}
			if (bo)
			{
				result.push_back(a);
			}
		}

		local_results.push_back(result);
	}
	vector<vector<unsigned int>> global_results;
	if (world_rank == 0) {
		global_results.resize(QueryNum);
	}

	for (int i = begin; i < QueryNum; i += world_size) {
		vector<unsigned int>& local_res = local_results[(i - begin) / world_size];
		int local_size = local_res.size();

		// Gather the size of the local result to the root process
		vector<int> all_sizes(world_size);
		MPI_Gather(&local_size, 1, MPI_INT, all_sizes.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

		// Gather the local result data to the root process
		vector<int> displs(world_size, 0);
		vector<unsigned int> recvbuf;
		if (world_rank == 0) {
			int total_size = 0;
			for (int j = 0; j < world_size; ++j) {
				displs[j] = total_size;
				total_size += all_sizes[j];
			}
			recvbuf.resize(total_size);
		}
		MPI_Gatherv(local_res.data(), local_size, MPI_UNSIGNED, recvbuf.data(), all_sizes.data(), displs.data(), MPI_UNSIGNED, 0, MPI_COMM_WORLD);

		if (world_rank == 0) {
			for (int j = 0; j < world_size; ++j) {
				global_results[i].insert(global_results[i].end(), recvbuf.begin() + displs[j], recvbuf.begin() + displs[j] + all_sizes[j]);
			}
			sort(global_results[i].begin(), global_results[i].end());
			global_results[i].erase(unique(global_results[i].begin(), global_results[i].end()), global_results[i].end());
		}
	}

	if (world_rank == 0) {
		// 结束计时
		QueryPerformanceCounter(&end);
		// 计算时间间隔
		interval = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
		cout << "代码执行时间: " << interval << " 秒" << endl;
	}

	MPI_Finalize();
	return 0;
}