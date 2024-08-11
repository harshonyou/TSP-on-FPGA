// icry

#include "toplevel.h"
#include <iostream>
#include <ap_fixed.h>
#include <iomanip>
#include <vector>

int main()
{
    uint32 ram[N * N];
    uint32 arg1 = 0, arg2 = 0, arg3 = 0, arg4 = 0;

    uint32 distances[N][N] = {
        {0, 10, 15, 20, 25},
        {10, 0, 35, 25, 30},
        {15, 35, 0, 30, 20},
        {20, 25, 30, 0, 10},
        {25, 30, 20, 10, 0}
    };

//    uint32 distances[N][N] = {
//		{0, 118, 123, 234, 185, 123},
//		{118, 0, 214, 56, 137, 46},
//		{123, 214, 0, 190, 187, 91},
//		{234, 56, 190, 0, 111, 99},
//		{185, 137, 187, 111, 0, 111},
//    	{95, 187, 127, 121, 127, 0}
//	};

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            ram[i * N + j] = distances[i][j];
        }
    }

    arg1 = 5;

    uint32 shortestDistance = toplevel(ram, &arg1, &arg2, &arg3, &arg4);

    std::cout << "Shortest trip distance: " << arg2 << "\n";
    std::cout << "Shortest trip perm: " << shortestDistance << "\n";

//    for (int i = 0; i < N+1; ++i) {
//    	std::cout << ram[i] << " ";
//	}

    return 0;
}
