#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <bitset>
#include <sys/time.h>
#include <arm_neon.h>
#include <mpi.h>
#define idlength 25205248

using namespace std;

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    int QueryNum = 1000;

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    int part = QueryNum / world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    struct timeval start, end, end_f;
    double interval;

    // 开始计时
    gettimeofday(&start, NULL);

    ifstream readIndex("ExpIndex", ios::binary | ios::in);
    if (!readIndex) {
        cerr << "无法打开ExpIndex文件" << endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
        MPI_Finalize();
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
        MPI_Abort(MPI_COMM_WORLD, 1);
        MPI_Finalize();
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
    if (world_rank == 0) {
        cout << "ExpIndex索引规模为1757" << endl;
        cout << "ExpIndex中最大文档ID为25205174,最小文档ID为0" << endl;
        cout << "ExpQuery最大查询规模为1000" << endl;
        gettimeofday(&end_f, NULL);
        interval = (end_f.tv_sec - start.tv_sec) + (end_f.tv_usec - start.tv_usec) / 1000000.0;
        cout << "读取文件时间: " << interval << " 秒" << endl;
        cout << "读取文件操作完成" << endl;
    }
    vector<vector<unsigned int>> local_results;
    int begin = part * world_rank;
    int final = (world_rank == world_size - 1) ? QueryNum : ((world_rank + 1) * part);
    for (int i = begin; i < final; i++) {
        int num = query[i].size();
        vector<unsigned int> result;
        bitset<idlength>* bits = new bitset<idlength>[num];
        for (int j = 0; j < num; j++) {
            for (size_t k = 0; k < index[query[i][j]].size(); k++) {
                bits[j].set(index[query[i][j]][k]);
            }
        }
        for (int j = 1; j < num; j++) {
            for (int k = 0; k < idlength / 128; k++) {
                uint8_t* data1 = (uint8_t*)&bits[0];
                uint8_t* add1 = data1 + 16 * k;
                uint8x16_t neon_data1 = vld1q_u8(add1);
                uint8_t* data2 = (uint8_t*)&bits[j];
                uint8_t* add2 = data2 + 16 * k;
                uint8x16_t neon_data2 = vld1q_u8(add2);
                neon_data1 = vandq_u8(neon_data1, neon_data2);
                vst1q_u8(add1, neon_data1);
            }
        }
        uint8_t* address = (uint8_t*)&bits[0];
        for (int j = 0; j < idlength / 128; j++) {
            bitset<128>* temp = (bitset<128>*)(address + j * 16);
            if (*temp != 0) {
                for (int k = j * 128; k < 128 * (j + 1); k++) {
                    if (bits[0][k]) {
                        result.push_back(k);
                    }
                }
            }
        }
        local_results.push_back(result);
        delete[] bits;
    }

    // Gather all local_results to the root process (rank 0)
    vector<vector<unsigned int>> global_results;
    if (world_rank == 0) {
        global_results.resize(QueryNum);
    }

    for (int i = begin; i < final; i++) {
        vector<unsigned int>& local_res = local_results[i - begin];
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
        gettimeofday(&end, NULL);
        // 计算时间间隔
        interval = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "代码执行时间: " << interval << " 秒" << endl;
    }
    MPI_Finalize();
    return 0;
}
