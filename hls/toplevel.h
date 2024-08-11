//Resource	Utilization		Available 	Utilization in %
//LUT		10804			17600		61.386364%
//LUTRAM	3008			6000		50.133335%
//FF		8503			35200		24.15625%
//BRAM		0.5				60			0.8333334%
//DSP		80				80			100.0%
//IO		15				100			15.000001%
//BUFG		1				32			3.125%

#ifndef TSP
#define TSP
#include <cstdint>
#include <ap_int.h>


// Typedefs
typedef unsigned int uint32;
typedef int int32;

// Custom bit-width integer types
typedef ap_uint<3> u3i;
typedef ap_uint<4> u4i;
typedef ap_uint<33> ufaci;
typedef ap_uint<12> ucani;
typedef uint8_t ui;
typedef uint16_t ui16;
typedef uint64_t ui64;

// Constants
const ui N = 13; // Number of nodes in the graph
const ui P = 4; // Number of parallel processing elements

// Declaration of the top level function that will interface with AXI4 and AXI Lite buses
uint32 toplevel(uint32 *ram, uint32 *arg1, uint32 *arg2, uint32 *arg3, uint32 *arg4);

#endif












// -------------------------------------------
// -------------------------------------------
// -------------------------------------------
// Alternative solution using Branch and Bound

//
//#ifndef __TOPLEVEL_H_
//#define __TOPLEVEL_H_
//
//#include <stdio.h>
//#include <stdlib.h>
//#include <ap_int.h>
//
//// Typedefs
//typedef unsigned int uint32;
//typedef int int32;
//
//// Constant for the number of cities
//#define N 5
//
//// Function prototypes
//uint32 toplevel(uint32 *ram, uint32 *arg1, uint32 *arg2, uint32 *arg3, uint32 *arg4);
//uint32 tsp_branch_and_bound(uint32 *graph, uint32 *result);
//
//#endif
