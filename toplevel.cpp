#include "toplevel.h"

//Input data storage
#define NUMDATA 100

uint32 inputdata[NUMDATA];

//Prototypes
uint32 addall(uint32 *data);
uint32 subfromfirst(uint32 *data);

uint32 toplevel(uint32 *ram, uint32 *arg1, uint32 *arg2, uint32 *arg3, uint32 *arg4) {
	#pragma HLS INTERFACE m_axi port=ram offset=slave bundle=MAXI
	#pragma HLS INTERFACE s_axilite port=arg1 bundle=AXILiteS
	#pragma HLS INTERFACE s_axilite port=arg2 bundle=AXILiteS
	#pragma HLS INTERFACE s_axilite port=arg3 bundle=AXILiteS
	#pragma HLS INTERFACE s_axilite port=arg4 bundle=AXILiteS
	#pragma HLS INTERFACE s_axilite port=return bundle=AXILiteS

	readloop: for(int i = 0; i < NUMDATA; i++) {
        #pragma HLS PIPELINE II=1
        inputdata[i] = ram[i];
	}

	*arg2 = addall(inputdata);
	*arg3 = subfromfirst(inputdata);

	return *arg1 + 1;
}

uint32 addall(uint32 *data) {
    uint32 total = 0;
    addloop: for(int i = 0; i < NUMDATA; i++) {
		#pragma HLS UNROLL factor=2
         #pragma HLS PIPELINE II=1
         total = total + data[i];
    }
    return total;
}

uint32 subfromfirst(uint32 *data) {
    uint32 total = data[0];
    subloop: for(int i = 1; i < NUMDATA; i++) {
		#pragma HLS UNROLL factor=2
         #pragma HLS PIPELINE II=1
         total = total - data[i];
    }
    return total;
}
