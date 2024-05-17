#include "toplevel.h"
#define NUMDATA 100

uint32 mainmemory[NUMDATA];

int main() {

    //Create input data
    for(int i = 0; i < NUMDATA; i++) {
    	mainmemory[i] = i;
    }
    mainmemory[0] = 8000;

    //Set up the slave inputs to the hardware
    uint32 arg1 = 0;
    uint32 arg2 = 0;
    uint32 arg3 = 0;
    uint32 arg4 = 0;

    //Run the hardware
    toplevel(mainmemory, &arg1, &arg2, &arg3, &arg4);

    //Read the slave outputs
    printf("Sum of input: %d\n", arg2);
    printf("Values 1 to %d subtracted from value 0: %d\n", NUMDATA-1, arg3);

    //Check the values are as expected
    if(arg2 == 12950 && arg3 == 3050) {
        return 0;
    } else {
        return 1; //An error!
    }
}
