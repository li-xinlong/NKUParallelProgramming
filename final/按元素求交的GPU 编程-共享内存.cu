#include<iostream>
#include<string>
#include<fstream>
#include<sstream>
#include<vector>
#include<algorithm>
#include<Windows.h>
#include<cuda_runtime.h>

using namespace std;

__global__ void FindKernel(const unsigned int* d_A, unsigned int a, bool* d_result, int size) {
	extern __shared__ unsigned int shared_A[];
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	int tid = threadIdx.x;

	if (idx < size) {
		shared_A[tid] = d_A[idx];
	}
	__syncthreads();

	if (idx < size) {
		if (shared_A[tid] == a) {
			*d_result = true;
		}
	}
}

bool FindCUDA(vector<unsigned int>& A, unsigned int a) {
	unsigned int* d_A;
	bool* d_result;
	bool h_result = false;
	int size = A.size() * sizeof(unsigned int);

	cudaMalloc((void**)&d_A, size);
	cudaMalloc((void**)&d_result, sizeof(bool));

	cudaMemcpy(d_A, A.data(), size, cudaMemcpyHostToDevice);
	cudaMemcpy(d_result, &h_result, sizeof(bool), cudaMemcpyHostToDevice);

	int blockSize = 256;
	int numBlocks = (A.size() + blockSize - 1) / blockSize;
	int sharedMemSize = blockSize * sizeof(unsigned int);

	FindKernel << <numBlocks, blockSize, sharedMemSize >> > (d_A, a, d_result, A.size());

	cudaMemcpy(&h_result, d_result, sizeof(bool), cudaMemcpyDeviceToHost);

	cudaFree(d_A);
	cudaFree(d_result);

	return h_result;
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
	int i = 0;
	while (readIndex.read(reinterpret_cast<char*>(&n), sizeof(n))) {
		vector<unsigned int> array(n);
		if (!readIndex.read(reinterpret_cast<char*>(array.data()), n * sizeof(unsigned int))) {
			cerr << "读取ExpIndex第" << i << "数组失败" << endl;
		}
		sort(array.begin(), array.end());
		index.push_back(array);
		i++;
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
	cout << "读取文件时间: " << interval << " 秒" << endl;
	cout << "读取文件操作完成" << endl;

	vector<vector<unsigned int>> results;
	for (int i = 0; i < QueryNum; i++) {
		int num = query[i].size();
		vector<unsigned int> result;
		for (int j = 0; j < index[query[i][0]].size(); j++) {
			bool bo = true;
			unsigned int a = index[query[i][0]][j];
			for (int k = 1; k < num; k++) {
				if (!FindCUDA(index[query[i][k]], a)) {
					bo = false;
					break;
				}
			}
			if (bo) {
				result.push_back(a);
			}
		}
		results.push_back(result);
	}

	QueryPerformanceCounter(&end);
	interval = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
	cout << "代码执行时间: " << interval << " 秒" << endl;

	return 0;
}
