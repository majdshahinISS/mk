
#include "Xil_Assert.h"
#include <stdio.h>


uint32_t Xil_AssertStatus;

int32_t Xil_AssertWait = 1;

void Xil_Assert(const char *File, int32_t Line)
{

	printf("Xil_Assert: File %s Line %d\n",File,Line);
	
	while (Xil_AssertWait != 0) {
	}
}

