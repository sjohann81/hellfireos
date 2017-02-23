/* this code is compiled as position independent code */

#include <hellfire.h>

void func(void){
	while(1){
		DEBUG_ADDR = (int32_t)'a';
	}
}
