#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <Windows.h>

using namespace std;

__global__ void FindKernel(unsigned int* A, unsigned int a, int size, bool* found) {
	extern __shared__ unsigned int shared_A[];
	int tid = threadIdx.x;
	int i = blockIdx.x * blockDim.x + threadIdx.x;

	if (i < size) {
		shared_A[tid] = A[i];
	}
	__syncthreads();

	if (tid < size && shared_A[tid] == a) {
		*found = true;
	}
}

bool Find(unsigned int* d_A, unsigned int a, int size) {
	bool* d_found;
	bool h_found = false;
	cudaMalloc((void**)&d_found, sizeof(bool));
	cudaMemcpy(d_found, &h_found, sizeof(bool), cudaMemcpyHostToDevice);

	int threadsPerBlock = 128;
	/*int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;*/
	int blocksPerGrid = 8;
	FindKernel << <blocksPerGrid, threadsPerBlock, threadsPerBlock * sizeof(unsigned int) >> > (d_A, a, size, d_found);

	cudaDeviceSynchronize();
	cudaMemcpy(&h_found, d_found, sizeof(bool), cudaMemcpyDeviceToHost);
	cudaFree(d_found);

	cudaError_t error = cudaGetLastError();
	if (error != cudaSuccess) {
		std::cerr << "CUDA error: " << cudaGetErrorString(error) << std::endl;
		return false;
	}

	return h_found;
}

int main() {
	cout << "ExpIndex索引规模为1757" << endl;
	cout << "ExpIndex中最大文档ID为25205174,最小文档ID为0" << endl;
	cout << "ExpQuery最大查询规模为1000" << endl;
	int QueryNum;
	cout << "请输入问题规模(0-1000内的整型)：" << endl;
	cin >> QueryNum;

	LARGE_INTEGER frequency, start, end, end_f;
	double interval;
	QueryPerformanceFrequency(&frequency);
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
			cerr << "读取ExpIndex失败" << endl;
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
	QueryPerformanceCounter(&end_f);
	interval = static_cast<double>(end_f.QuadPart - start.QuadPart) / frequency.QuadPart;
	std::cout << "读取文件时间: " << interval << " 秒" << std::endl;
	cout << "读取文件操作完成" << endl;

	vector<vector<unsigned int>> results;
	for (int i = 0; i < QueryNum; i++) {
		int num = query[i].size();
		vector<unsigned int> result = index[query[i][0]];

		for (int j = 1; j < num; j++) {
			unsigned int* d_index;
			cudaMalloc((void**)&d_index, index[query[i][j]].size() * sizeof(unsigned int));
			cudaMemcpy(d_index, index[query[i][j]].data(), index[query[i][j]].size() * sizeof(unsigned int), cudaMemcpyHostToDevice);

			for (int k = 0; k < result.size(); k++) {
				if (!Find(d_index, result[k], index[query[i][j]].size())) {
					result.erase(result.begin() + k);
					k--;
				}
			}
			cudaFree(d_index);
		}
		results.push_back(result);
	}
	QueryPerformanceCounter(&end);
	interval = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
	std::cout << "代码执行时间: " << interval << " 秒" << std::endl;
	return 0;
}
