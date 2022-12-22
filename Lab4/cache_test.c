#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#define ARRAY_SIZE (1 << 30)                                    // test array size is 2^28

typedef unsigned char BYTE;										// define BYTE as one-byte type

BYTE array[ARRAY_SIZE];											// test array
const int L1_CACHE_SIZE = 1 << 17;
const int L2_cache_size = 1 << 18;

double get_usec(const struct timeval tp0, const struct timeval tp1)
{
    return 1000000 * (tp1.tv_sec - tp0.tv_sec) + tp1.tv_usec - tp0.tv_usec;
}

// have an access to arrays with L2 Data Cache'size to clear the L1 cache
void Clear_L1_Cache()
{
    memset(array, 0, L2_cache_size);
}

// have an access to arrays with ARRAY_SIZE to clear the L2 cache
void Clear_L2_Cache()
{
    memset(array, 0, ARRAY_SIZE);
}

void Test_Cache_Size()
{
    printf("**************************************************************\n");
    printf("Cache Size Test\n");

    double avgTime[50];

    for (size_t i = 2; i <= 8; i++) {                          // 测试数组的大小范围
        size_t array_size = 1 << i;                            // 单位为KB
        size_t step_forward = 128;                             // 每次访存的步进

        Clear_L1_Cache();
        Clear_L2_Cache();

        struct timeval tp[2];
        gettimeofday(&tp[0], NULL);
        for (size_t j = 0; j < (1 << 12) >> i; j++) {
            for (size_t k = 0; k < (array_size << 10); k += step_forward) {
                array[k]++;
            }
        }
        gettimeofday(&tp[1], NULL);
        avgTime[i] = get_usec(tp[0], tp[1]);

        printf("[Test Array Size = %3dKB]\tAverage Access Time = %.3lfus\n", (int) (1 << i), avgTime[i]);
    }

    double timeDif = 0;
    int cacheSize = 0;
    for (int i = 2; i < 8; i++) {
        if (avgTime[i + 1] - avgTime[i] > timeDif) {
            cacheSize = i;
            timeDif = avgTime[i + 1] - avgTime[i];
        }
    }

    printf("L1 Data Cache Size = %dKB\n", (1 << cacheSize));
}

void Test_L1C_Block_Size()
{
    printf("**************************************************************\n");
    printf("L1 DCache Block Size Test\n");

    Clear_L1_Cache();
    Clear_L2_Cache();			// Clear L1 Cache

    struct timeval tp[2];
    double avgTime[20];

    for (size_t i = 2; i <= 8; i++) {
        size_t block_size = 1 << i;         // 本次尝试的块大小
        Clear_L1_Cache();
        Clear_L2_Cache();
        gettimeofday(&tp[0], NULL);
        for (size_t j = 0; j < (1 << i); j++) {   // 保持访存次数不变
            for (size_t k = 0; k < L1_CACHE_SIZE; k += block_size) {
                array[k]++;
            }
        }
        gettimeofday(&tp[1], NULL);

        avgTime[i] = get_usec(tp[0], tp[1]);

        printf("[Test_Array_Jump = %3dB]\tAverage Access Time = %.3lfus\n", (int) block_size, avgTime[i]);
    }

    double timeDif = 0;
    int block_size = 0;
    for (int i = 2; i < 8; i++) {
        if (avgTime[i + 1] - avgTime[i] > timeDif) {
            block_size = i;
            timeDif = avgTime[i + 1] - avgTime[i];
        }
    }

    printf("L1 Data Cache Block is %dB\n", 1 << block_size);
}

void Test_L1C_Way_Count()
{
    printf("**************************************************************\n");
    printf("L1 DCache Way Count Test\n");

    double avgTime[20];
	
    for (size_t n = 1; n <= 8; n++) {
        size_t set_size = (L1_CACHE_SIZE << 1) / (1 << n);

        Clear_L1_Cache();
        Clear_L2_Cache();

        struct timeval tp[2];

        gettimeofday(&tp[0], NULL);
        for (int j = 1; j < (1 << n); j += 2) {
            for (int k = 0; k < set_size; k++) {
                array[k + j * set_size]++;
            }
            gettimeofday(&tp[1], NULL);
        }
        gettimeofday(&tp[1], NULL);
        avgTime[n] = get_usec(tp[0], tp[1]);

        printf("[Test Split Groups = %3d]\tAverage Access Time = %.3lfus\n", (int) 1 << n, avgTime[n]);
    }
}

void Test_TLB_Size()
{
    printf("**************************************************************\n");
    printf("TLB Size Test\n");

    const size_t page_size = 1 << 14;			// Execute "getconf PAGE_SIZE" under linux terminal
    double avgTime[20];

    for (size_t i = 1; i <= 8; i++) {
        size_t entries_count = 1 << i;
        struct timeval tp[2];
        Clear_L1_Cache();
        Clear_L2_Cache();

        gettimeofday(&tp[0], NULL);
        for (size_t j = 0; j < (1 << 12) >> i; j++) {
            for (size_t k = 0; k < entries_count; k++) {
                array[k * page_size]++;
            }
        }
        gettimeofday(&tp[1], NULL);
        double access_time = get_usec(tp[0], tp[1]);

        printf("[Test TLB Entries = %3d]\tAverage Access Time = %.3lfns\n", (int) entries_count, access_time);
    }
}

int main()
{
	 Test_Cache_Size();
	 Test_L1C_Block_Size();
	// Test_L2C_Block_Size();
	 Test_L1C_Way_Count();
	// Test_L2C_Way_Count();
	// Test_Cache_Write_Policy();
	// Test_Cache_Swap_Method();
	Test_TLB_Size();
	
	return 0;
}
