#include "toplevel.h"

// Precomputed factorial values used for permutation indexing
constexpr long int factorial_table[N] = {
  1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800, 479001600//, 6227020800
};
// Last fact is commented out as N is set to 13

// Maximal value for ucani, used as a high value to indicate invalid states
constexpr ui16 MAXUCANI = 4095;

// Function to compute the total distance of a given permutation path in the graph
ucani getDistance(const u4i perm[N], const ui distances[N][N]) {
	#pragma HLS INLINE

	ucani ret = 0;
	distanceloop: for(u4i i = 0; i < N-1; ++i) {
		#pragma HLS UNROLL
		ret += distances[perm[i]][perm[i+1]]; // Accumulate distance between consecutive nodes
	}
	ret += distances[perm[N-1]][perm[0]]; // Add distance back to the starting node
	return ret;
}

// Generates a permutation based on an index using factoradic number system
// It is used for unique permutation generation as it's suitable for parallel execution
void getPermutation(ufaci i, u4i *perm) {
	#pragma HLS INLINE
	#pragma HLS BIND_STORAGE variable=perm type=ram_1p impl=bram

	posloop: for (u4i k = 0; k < N; ++k) {
		perm[k] = i / factorial_table[N - 1 - k];
		i = i % factorial_table[N - 1 - k];
	}

	validateloop: for (char k = N - 1; k > 0; --k)
		innervalidateloop: for (char j = k - 1; j >= 0; --j)
			perm[k] += (perm[j] <= perm[k]);
}

// Computes the shortest path for a specific set of distances and permutation index
ucani compute(const ufaci i, const ui distances[N][N]) {
	#pragma HLS INLINE

	u4i perm[N] = {0};
	#pragma HLS BIND_STORAGE variable=perm type=ram_1p impl=bram

	getPermutation(i, perm);

	 // Optimization to ignore symmetrical permutations
    if (perm[1] < perm[N-1]) {
        return getDistance(perm, distances);
    } else {
        return MAXUCANI; // Ignore this permutation by returning max value
    }
}

// Main function to compute the shortest path using hardware acceleration
uint32 toplevel(uint32 *ram, uint32 *arg1, uint32 *arg2, uint32 *arg3, uint32 *arg4) {
	#pragma HLS INTERFACE m_axi port=ram offset=slave bundle=MAXI
	#pragma HLS INTERFACE s_axilite port=arg1 bundle=AXILiteS register
	#pragma HLS INTERFACE s_axilite port=arg2 bundle=AXILiteS register
	#pragma HLS INTERFACE s_axilite port=arg3 bundle=AXILiteS register
	#pragma HLS INTERFACE s_axilite port=arg4 bundle=AXILiteS register
	#pragma HLS INTERFACE s_axilite port=return bundle=AXILiteS register

	ui distances[P][N][N];

	#pragma HLS BIND_STORAGE variable=distances type=ram_1wnr impl=lutram
	#pragma HLS ARRAY_PARTITION variable=distances complete dim=1

	 // Load distance matrix from external RAM to local storage
	loadloop: for (ui i = 0; i < N * N; ++i) {
		#pragma HLS pipeline II=1
		innerloadloop: for (u3i j = 0; j < P; ++j) {
			#pragma HLS UNROLL
			distances[j][i / N][i % N] = ram[i];
		}
    }

    constexpr long int factorialN = factorial_table[N - 1];

    ucani candidates[P];
    ufaci bestIndex[P];

	#pragma HLS ARRAY_PARTITION variable=candidates dim=1 complete
	#pragma HLS ARRAY_PARTITION variable=bestIndex dim=1 complete

    // Initialize candidate distances to maximal value
    // and candidate optimal permutations to zero
    for (ui j = 0; j < P; ++j) {
		candidates[j] = MAXUCANI;
		bestIndex[j] = 0;
	}

    // Compute shortest paths using permutations by leveraging unrolled loops and pipelining
    mainloop: for (ufaci i_ = 0; i_ < factorialN; i_ += P) {
		#pragma HLS pipeline II=1

    	innermainloop: for (u3i j = 0; j < P; ++j) {
		#pragma HLS UNROLL
    		ucani tempDistance = compute(i_+j, distances[j]);
			if (candidates[j] > tempDistance) {
				candidates[j] = tempDistance;
				bestIndex[j] = i_ + j;
			}
		}
    }

    // Determine the overall shortest path
    ui16 shortestDistance = MAXUCANI;
    ufaci globalBestIndex = 0;

    // Find best candidate
    candidateloop: for (u3i j = 0; j < P; ++j) {
		#pragma HLS pipeline II=2
		if (shortestDistance > candidates[j]) {
			shortestDistance = candidates[j];
			globalBestIndex = bestIndex[j];
		}
	}

    *arg2 = shortestDistance; // Return the shortest distance
    *arg3 = globalBestIndex; // Return the index of the best permutation

    return globalBestIndex;
}












// -------------------------------------------
// -------------------------------------------
// -------------------------------------------
// Alternative solution using Branch and Bound

//#include "toplevel.h"
//
//#define NUMDATA 100
//
//uint32 inputdata[NUMDATA];
//
//uint32 min_cost = 4294967295; // Maximum value for uint32
//uint32 final_path[NUMDATA];
//
//// Prototypes
//uint32 tsp_branch_and_bound(uint32 *graph, uint32 num_cities, uint32 *result);
//void copy_to_final(uint32 curr_path[], uint32 n);
//uint32 first_min(uint32 *graph, uint32 i, uint32 n);
//uint32 second_min(uint32 *graph, uint32 i, uint32 n);
//void tsp_iterative(uint32 *graph, uint32 num_cities, uint32 curr_bound, uint32 curr_weight, uint32 curr_path[], uint32 visited[]);
//
//uint32 toplevel(uint32 *ram, uint32 *arg1, uint32 *arg2, uint32 *arg3, uint32 *arg4) {
//    #pragma HLS INTERFACE m_axi port=ram offset=slave bundle=MAXI
//    #pragma HLS INTERFACE s_axilite port=arg1 bundle=AXILiteS
//    #pragma HLS INTERFACE s_axilite port=arg2 bundle=AXILiteS
//    #pragma HLS INTERFACE s_axilite port=arg3 bundle=AXILiteS
//    #pragma HLS INTERFACE s_axilite port=arg4 bundle=AXILiteS
//    #pragma HLS INTERFACE s_axilite port=return bundle=AXILiteS
//
//    uint32 num_cities = *arg1;
//    uint32 result[NUMDATA];
//
//    tsp_branch_and_bound(ram, num_cities, result);
//
//    for (int i = 0; i < num_cities; i++) {
//        ram[i] = result[i];
//    }
//
//    *arg2 = min_cost;
//
//    return min_cost;
//}
//
//uint32 tsp_branch_and_bound(uint32 *graph, uint32 num_cities, uint32 *result) {
//    uint32 curr_path[NUMDATA];
//    uint32 visited[NUMDATA];
//
//    for (uint32 i = 0; i < num_cities; i++) {
//        visited[i] = 0;
//        curr_path[i] = -1;
//    }
//
//    uint32 curr_bound = 0;
//    for (uint32 i = 0; i < num_cities; i++) {
//        curr_bound += (first_min(graph, i, num_cities) + second_min(graph, i, num_cities));
//    }
//
//    curr_bound = (curr_bound & 1) ? curr_bound / 2 + 1 : curr_bound / 2;
//
//    visited[0] = 1;
//    curr_path[0] = 0;
//
//    tsp_iterative(graph, num_cities, curr_bound, 0, curr_path, visited);
//
//    for (uint32 i = 0; i < num_cities; i++) {
//        result[i] = final_path[i];
//    }
//
//    return min_cost;
//}
//
//void copy_to_final(uint32 curr_path[], uint32 n) {
//    for (uint32 i = 0; i < n; i++) {
//        final_path[i] = curr_path[i];
//    }
//    final_path[n] = curr_path[0];
//}
//
//uint32 first_min(uint32 *graph, uint32 i, uint32 n) {
//    uint32 min = 4294967295;
//    for (uint32 k = 0; k < n; k++) {
//        if (graph[i * n + k] < min && i != k) {
//            min = graph[i * n + k];
//        }
//    }
//    return min;
//}
//
//uint32 second_min(uint32 *graph, uint32 i, uint32 n) {
//    uint32 first = 4294967295, second = 4294967295;
//    for (uint32 j = 0; j < n; j++) {
//        if (i == j) continue;
//
//        if (graph[i * n + j] <= first) {
//            second = first;
//            first = graph[i * n + j];
//        } else if (graph[i * n + j] <= second && graph[i * n + j] != first) {
//            second = graph[i * n + j];
//        }
//    }
//    return second;
//}
//
//void tsp_iterative(uint32 *graph, uint32 num_cities, uint32 curr_bound, uint32 curr_weight, uint32 curr_path[], uint32 visited[]) {
//    struct Node {
//        uint32 level;
//        uint32 curr_bound;
//        uint32 curr_weight;
//        uint32 curr_path[NUMDATA];
//        uint32 visited[NUMDATA];
//    };
//
//    Node stack[NUMDATA * NUMDATA];
//    int top = -1;
//
//    stack[++top] = {1, curr_bound, curr_weight, {0}, {0}};
//    for (uint32 i = 0; i < num_cities; i++) {
//        stack[top].curr_path[i] = curr_path[i];
//        stack[top].visited[i] = visited[i];
//    }
//
//    enum State { INIT, CHECK_LEVEL, ITERATE_CITIES, UPDATE_STATE, BACKTRACK, FINALIZE };
//    State state = INIT;
//    Node node;
//    uint32 i = 0;
//    uint32 curr_res = 0;
//
//    while (true) {
//        #pragma HLS PIPELINE II=1
//        switch (state) {
//            case INIT:
//                if (top < 0) state = FINALIZE; // Exit condition
//                else {
//                    node = stack[top--];
//                    i = 0;
//                    state = CHECK_LEVEL;
//                }
//                break;
//
//            case CHECK_LEVEL:
//                if (node.level == num_cities) {
//                    if (graph[node.curr_path[node.level - 1] * num_cities + node.curr_path[0]] != 0) {
//                        curr_res = node.curr_weight + graph[node.curr_path[node.level - 1] * num_cities + node.curr_path[0]];
//                        state = UPDATE_STATE;
//                    } else {
//                        state = INIT;
//                    }
//                } else {
//                    state = ITERATE_CITIES;
//                }
//                break;
//
//            case UPDATE_STATE:
//                if (curr_res < min_cost) {
//                    copy_to_final(node.curr_path, num_cities);
//                    min_cost = curr_res;
//                }
//                state = INIT;
//                break;
//
//            case ITERATE_CITIES:
//                if (i < num_cities) {
//                    if (graph[node.curr_path[node.level - 1] * num_cities + i] != 0 && node.visited[i] == 0) {
//                        uint32 temp = node.curr_bound;
//                        node.curr_weight += graph[node.curr_path[node.level - 1] * num_cities + i];
//
//                        if (node.level == 1) {
//                            node.curr_bound -= ((first_min(graph, node.curr_path[node.level - 1], num_cities) + first_min(graph, i, num_cities)) / 2);
//                        } else {
//                            node.curr_bound -= ((second_min(graph, node.curr_path[node.level - 1], num_cities) + first_min(graph, i, num_cities)) / 2);
//                        }
//
//                        if (node.curr_bound + node.curr_weight < min_cost) {
//                            node.curr_path[node.level] = i;
//                            node.visited[i] = 1;
//                            stack[++top] = {node.level + 1, node.curr_bound, node.curr_weight, {0}, {0}};
//                            for (uint32 j = 0; j < num_cities; j++) {
//                                stack[top].curr_path[j] = node.curr_path[j];
//                                stack[top].visited[j] = node.visited[j];
//                            }
//                        }
//
//                        node.curr_weight -= graph[node.curr_path[node.level - 1] * num_cities + i];
//                        node.curr_bound = temp;
//
//                        for (uint32 k = 0; k < num_cities; k++) {
//                            node.visited[k] = 0;
//                        }
//                        for (uint32 j = 0; j <= node.level - 1; j++) {
//                            node.visited[node.curr_path[j]] = 1;
//                        }
//                    }
//                    i++;
//                } else {
//                    state = INIT;
//                }
//                break;
//
//            case FINALIZE:
//                return;
//        }
//    }
//}
