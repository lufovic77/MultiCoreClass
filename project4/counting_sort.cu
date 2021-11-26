#include <cuda.h>
// 나중에 지우기
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

__host__ void counting_sort(int arr[], int size, int max_val)
{
   int i, j;
   // fill in 
   int histogram[max_val];

   for (i=0; i<max_val; i++){
      histogram[i] = 0;
   }
   for (i=0; i<size; i++){
      histogram[arr[i]]++;
   }
   int idx = 0;
   for (i=0;i<max_val;i++){
      for (j=0;j<histogram[i];j++){
            arr[idx++] = i;
      }
   }
}