/*
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

/*
 To compile: 
    /usr/local/cuda/bin/nvcc -arch=sm_30 reduction_kernel.cu
 To run with the array size 2^20, expo dist mean 5, and init seed 17:
    ./a.out 20 5 17
 */
#include <stdio.h>
#include <stdlib.h>



extern "C" void reduce_wrapper(int *n, int *mean, int *seed, double *max_val);



template<class T>
struct SharedMemory
{
    __device__ inline operator       T *()
    {
        extern __shared__ int __smem[];
        return (T *)__smem;
    }

    __device__ inline operator const T *() const
    {
        extern __shared__ int __smem[];
        return (T *)__smem;
    }
};

//run CPU version

double 
reduceCPU(double *data, int size)
{   double cpu_max = -1.0;
    for (unsigned int i = 1; i < size; i++)
    {  
        if (data[i] > cpu_max)
            cpu_max = data[i];           
    }
    return cpu_max;
}

/*
    Parallel sum reduction using shared memory
    - takes log(n) steps for n input elements
    - uses n threads
    - only works for power-of-2 arrays
*/

/*
    This version uses sequential addressing -- no divergence or bank conflicts.
*/
__global__ void
reduce(double *g_idata, double *max, int *mutex, unsigned int n)
{
    double *sdata = SharedMemory<double>();

    // load shared mem
    unsigned int tid = threadIdx.x;
    unsigned int i = blockIdx.x*blockDim.x + threadIdx.x;
    unsigned int stride = gridDim.x*blockDim.x;
    unsigned int offset = 0;
    // sdata[tid] = (i < n) ? g_idata[i] : 0;
    float local_max = -1.0;
    while( i+offset < n){
        local_max = fmaxf(local_max, g_idata[i+offset]);
        offset += stride;
    }
    sdata[tid] = local_max;
    __syncthreads();

    // do reduction in shared mem
    for (unsigned int s=blockDim.x/2; s>0; s>>=1)
    {
        if (tid < s)
        {
            sdata[tid] = fmaxf(sdata[tid], sdata[tid + s]);
        }
        __syncthreads();
    }

    // write result for this block to global mem
    if (tid == 0){
    while(atomicCAS(mutex, 0, 1) != 0); //lock
    *max = fmaxf(*max, sdata[0]);
    atomicExch(mutex, 0); //unlock
    }
}


// CUDA Runtime
#include <cuda_runtime.h>

#define checkCudaErrors(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)
{
   if (code != cudaSuccess) 
   {
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

void
reduce_wrapper(int *n, int *mean, int *seed, double *max_val)
{
        // int n = atoi(argv[1]); 
        // int mean = atoi(argv[2]);
        // int seed = atoi(argv[3]); 
        int size = 1 << *n;    // number of elements to reduce
        int maxThreads = 256;  // number of threads per block

        // create random input data on CPU
        unsigned int bytes = size * sizeof(double);

        double *h_idata = (double *) malloc(bytes);
        double *h_max = (double *) malloc(sizeof(double));  /*value to hold result from device */

        srand48(*seed);
        for (int i=0; i<size; i++)
        {
                // h_idata[i] = 1.0; // for testing
                // expo dist with mean 5.0
                h_idata[i] = -(*mean) * log(drand48());
        }

        int numBlocks = size / maxThreads;
        int numThreads = size;

        int smemSize = maxThreads * sizeof(double);

        // allocate device memory and data
        double  *d_idata = NULL;
        double *d_max;
        int *d_mutex;   /*mutex write protection in shared memory */
     

        checkCudaErrors(cudaMalloc((void **) &d_idata, bytes));
        checkCudaErrors(cudaMalloc((void **) &d_max, sizeof(double)));
        checkCudaErrors(cudaMalloc((void **) &d_mutex, sizeof(int)));

        // initialize d_max to be 0
        checkCudaErrors(cudaMemset(d_max, 0, sizeof(double)));

        //set Mutex to be unlocked
        checkCudaErrors(cudaMemset(d_mutex, 0, sizeof(int))); 

        // copy data directly to device memory
        checkCudaErrors(cudaMemcpy(d_idata, h_idata, bytes, cudaMemcpyHostToDevice));
        //call reduce
        reduce<<<numBlocks,maxThreads,smemSize>>>(d_idata, d_max, d_mutex, numThreads);

        checkCudaErrors(cudaMemcpy(h_max, d_max, sizeof(double), cudaMemcpyDeviceToHost));

        printf("GPU max : %f\n", *h_max);

        //return h_max to call function in C
        *max_val = *h_max;

        //free cuda memory
        checkCudaErrors(cudaFree(d_idata));
        checkCudaErrors(cudaFree(d_max));
        
        double cpu_max;
        cpu_max = reduceCPU(h_idata, size);
   
        printf("CPU max : %f\n", cpu_max);

}
