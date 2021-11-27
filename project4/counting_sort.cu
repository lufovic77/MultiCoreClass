#include <cuda.h>
// 나중에 지우기
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdio>
using namespace std;

__global__ void countingSortKernel(int * histogram_d , int * array_d, int max_val_h, int size, int num_blocks, int num_threads){
   int idx = blockIdx.x * blockDim.x + threadIdx.x;
   int total_threads = num_blocks*num_threads;
   int interval = size / total_threads;
   int start = idx*interval, end;
   if(blockIdx.x == num_blocks-1 && threadIdx.x == num_threads-1)
      end = size;
   else 
      end = start+interval;
   __syncthreads();
   for(int i=start;i<end;i++)
      atomicAdd(&histogram_d[array_d[i]], 1);
   __syncthreads();
}

__host__ void counting_sort(int arr[], int size, int max_val)
{
   //int i, j;
   int num_blocks = 3, num_threads = 10;
   // fill in 
   int arrSize = size*sizeof(int), histoSize = max_val*sizeof(int);
   int * histogram_d;
   int * histogram;
   int * arr_d;
   histogram = new int[histoSize];
   // Initializing histogram
   cudaMalloc(&histogram_d, histoSize);
	cudaMemset(histogram_d, 0, histoSize);
   cudaMalloc(&arr_d, arrSize);
   cudaMemcpy(arr_d, arr, arrSize, cudaMemcpyHostToDevice);

   countingSortKernel <<<num_blocks, num_threads>>> (histogram_d, arr_d, max_val, size, num_blocks, num_threads);
   cudaMemcpy(histogram, histogram_d, histoSize, cudaMemcpyDeviceToHost);
   int idx = 0;
   // copy the output result
   for (int i=0;i<max_val;i++){
      for (int j=0;j<histogram[i];j++){
            arr[idx++] = i;
      }
   }
   cudaFree(histogram);
   cudaFree(histogram_d);
   cudaFree(arr_d);
}