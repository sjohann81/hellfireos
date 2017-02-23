#include <hellfire.h>

typedef void (*fpt)(void);
/*
void func(void){
	while(1){
		DEBUG_ADDR = (int32_t)'a';
	}
}
*/

void app_main(void){
	fpt function;
	
	function = (void *)malloc(5000);
//	memcpy(function, func, 4000);
	
	hf_spawn(function, 0, 0, 0, "Consumer C", 1024);
}
