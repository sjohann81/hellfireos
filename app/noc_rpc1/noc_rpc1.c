#include <hellfire.h>
#include <noc.h>
#include <noc_rpc.h>

struct calc_s {
	int op;
	int a;
	int b;
	int res;
};

int32_t calc(int8_t *in, int8_t *out)
{
	struct calc_s *data_in, *data_out;
	
	data_in = (struct calc_s *)in;
	data_out = (struct calc_s *)out;
	
	printf("\n%d (%d) %d: ", data_in->a, data_in->op, data_in->b);
	
	switch (data_in->op) {
		case 0:
			data_out->res = data_in->a + data_in->b;
			break;
		case 1:
			data_out->res = data_in->a - data_in->b;
			break;
		case 2:
			data_out->res = data_in->a * data_in->b;
			break;
		case 3:
			data_out->res = data_in->a / data_in->b;
			break;
		default:
			data_out->op = -1;
			data_out->res = 0;
			return -1;
	};
	data_out->op = 0;
	
	printf("%d", data_out->res);

	return 0;
}

void thread(void)
{
	int16_t i, j;
	int32_t r;
	uint32_t cycles;
	int8_t op;
	struct calc_s input, result;

	if (hf_comm_create(hf_selfid(), 1000, 0))
		panic(0xff);

	delay_ms(50);

	while (1){
		for (i = 0; i < hf_ncores(); i++) {
			if (i != hf_cpuid()) {
				for (j = 0; j < 4; j++) {
					input.op = j;
					input.a = 7 + i;
					input.b = 3 + i;
					cycles = _readcounter();
					
					r = hf_call(i, 0, 0, (int8_t *)&input, sizeof(struct calc_s), (int8_t *)&result, sizeof(struct calc_s));
					
					cycles = _readcounter() - cycles;
					switch (j) {
						case 0: op = '+'; break;
						case 1: op = '-'; break;
						case 2: op = '*'; break;
						case 3: op = '/'; break;
					};
					printf("(r: %d) - %d %c %d = %d, %d cycles\n", r, input.a, op, input.b, result.res, cycles);
				}
			}
		}
		
		while(1);
	}
}

void bogus(void){
	while (1);
}

void app_main(void)
{
	if (hf_cpuid() == 0){
		hf_spawn(thread, 0, 0, 0, "app task", 4096);
	}else{
		hf_register(0, 0, calc, sizeof(struct calc_s), sizeof(struct calc_s));

		hf_spawn(bogus, 0, 0, 0, "bog1", 1024);
		hf_spawn(bogus, 0, 0, 0, "bog2", 1024);
		hf_spawn(bogus, 0, 0, 0, "bog3", 1024);
		hf_spawn(bogus, 0, 0, 0, "bog4", 1024);
		hf_spawn(bogus, 0, 0, 0, "bog5", 1024);
		hf_spawn(bogus, 0, 0, 0, "bog6", 1024);
	}	
}
