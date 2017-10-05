#include <hellfire.h>

float epsilon(void){
	float x = 1.0f;
	
	while ((1.0f + (x / 2.0f)) > 1.0f)
		x = x / 2.0f;
		
	return x;
}

void doit(float a, float x){
	printf("\n\na = %f, x = %f\n", a, x);
	printf("\nx                                            = %.9f", x);
	printf("\n(a + x) - a                                  = %.9f", (a + x) - a);
	printf("\n4.0 * (a + x) - 3 * x - 3                    = %.9f", 4.0f * (a + x) - 3.0f * x - 3.0f);
	printf("\n0.2 * (a + x) + 0.8 * x - 0.15               = %.9f", 0.2f * (a + x) + 0.8f * x - 0.15f);
	printf("\nx / (a + x)                                  = %.9f", x / (a + x));
	printf("\n1.0 - a / (a + x)                            = %.9f", 1.0f - a / (a + x));
	printf("\n1.0 / ( a / x + 1)                           = %.9f", 1.0f / ( a / x + 1));
	printf("\n(1.0 - (a + 0.1 * x) / (a + x)) / 0.9        = %.9f", (1.0f - (a + 0.1f * x) / (a + x)) / 0.9f);
	printf("\na / ( a + x)                                 = %.9f", a / ( a + x));
	printf("\n1.0 - x / (a + x)                            = %.9f", 1.0f - x / (a + x));
	printf("\n1.0 / (1 + x / a)                            = %.9f", 1.0f / (1.0f + x / a));
	printf("\n(a + x / 10.0) / (a + x) - 0.1 * x / (a + x) = %.9f", (a + x / 10.0f) / (a + x) - 0.1f * x / (a + x));
	printf("\nlog((a + x) / a))                            = %.9f", log((a + x) / a));
	printf("\nlog(a + x) - log(a)                          = %.9f", log(a + x) - log(a));
	printf("\nlog(1.0 + x / a)                             = %.9f", log(1.0f + x / a));
	printf("\n-log(1.0 - x / (a + x))                      = %.9f", -log(1.0f - x / (a + x)));
	printf("\n");
}

void testfp(void){
	int i;
	float a, x;

	printf("\nmachine epsilon: %.9f", epsilon());

	a = 3.0f / 4.0f;
	for (i = -10; i > -13; i--){
		x = pow(3.0f, i);
		doit(a, x);
	}

}

int fac(int n){
	int i, f;

	for (i = 1, f = 1; i <= n; i++)
		f = f * i;

	return f;
}

int euler(){
	int i;
	float e, f;

	e = 0;
	for (i = 0; i < 13; i++){
		e += 1.0f / (float)fac(i);
		f = e;
		printf("[%d] - e = %.9f\n", i, f);
	}

	return 0;
}


void task(){
	uint32_t i = 0;
	
	while (1){
		printf("\ntest %d", i++);
		testfp();
		euler();
		delay_ms(1000);
	}
}

void app_main(void)
{
	hf_spawn(task, 0, 0, 0, "task", 2048);
}
