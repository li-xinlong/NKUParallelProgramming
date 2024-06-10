#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <Windows.h>
#include <cuda_runtime.h>
using namespace std;
__global__ void findKernel(const unsigned int* index, const unsigned int* query, bool* results, int indexSize, int querySize) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < indexSize) {
        for (int i = 0; i < querySize; i++) {
            if (index[idx] == query[i]) {
                results[idx] = true;
                return;
            }
        }
    }
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
    cout << "读取文件时间: " << interval << " 秒" << endl;
    cout << "读取文件操作完成" << endl;

    vector<vector<unsigned int>> results;

    for (int i = 0; i < QueryNum; i++) {
        int num = query[i].size();
        vector<unsigned int> result = index[query[i][0]];
        for (int j = 1; j < num; j++) {
            unsigned int* d_index;
            unsigned int* d_query;
            bool* d_results;
            int indexSize = index[query[i][j]].size();
            int querySize = result.size();

            cudaMalloc((void**)&d_index, indexSize * sizeof(unsigned int));
            cudaMalloc((void**)&d_query, querySize * sizeof(unsigned int));
            cudaMalloc((void**)&d_results, indexSize * sizeof(bool));
            
            cudaMemcpy(d_index, index[query[i][j]].data(), indexSize * sizeof(unsigned int), cudaMemcpyHostToDevice);
            cudaMemcpy(d_query, result.data(), querySize * sizeof(unsigned int), cudaMemcpyHostToDevice);
            cudaMemset(d_results, 0, indexSize * sizeof(bool));
            
            int blockSize = 256;
            int numBlocks = (indexSize + blockSize - 1) / blockSize;
            findKernel<<<numBlocks, blockSize>>>(d_index, d_query, d_results, indexSize, querySize);
            cudaDeviceSynchronize();
            
            bool* h_results = new bool[indexSize];
            cudaMemcpy(h_results, d_results, indexSize * sizeof(bool), cudaMemcpyDeviceToHost);
            
            vector<unsigned int> temp_result;
            for (int k = 0; k < indexSize; k++) {
                if (h_results[k]) {
                    temp_result.push_back(result[k]);
                }
            }
            result = temp_result;

            cudaFree(d_index);
            cudaFree(d_query);
            cudaFree(d_results);
            delete[] h_results;
        }
        results.push_back(result);
    }

    QueryPerformanceCounter(&end);
    interval = static_cast<double>(end.QuadPart - end_f.QuadPart) / frequency.QuadPart;
    cout << "代码执行时间: " << interval << " 秒" << endl;
    for (int i = 0; i < results.size(); i++) {
        cout << results[i].size() << endl;
    }
    return 0;
}
