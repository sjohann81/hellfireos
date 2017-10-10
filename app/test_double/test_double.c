#include <hellfire.h>

double epsilon(void){
	double x = 1.0;
	
	while ((1.0 + (x / 2.0)) > 1.0)
		x = x / 2.0;
		
	return x;
}

void doit(double a, double x){
	char buf[30], buf2[30];
	
	ftoa(a, buf, 6); ftoa(x, buf2, 6);				printf("\n\na = %s, x = %s\n", buf , buf2);
	ftoa(x, buf, 6);						printf("\nx                                            = %s", buf);
	ftoa((a + x) - a, buf, 6);					printf("\n(a + x) - a                                  = %s", buf);
	ftoa(4.0 * (a + x) - 3.0 * x - 3.0, buf, 6);			printf("\n4.0 * (a + x) - 3 * x - 3                    = %s", buf);
	ftoa(0.2 * (a + x) + 0.8 * x - 0.15, buf, 6);			printf("\n0.2 * (a + x) + 0.8 * x - 0.15               = %s", buf);
	ftoa(x / (a + x), buf, 6);					printf("\nx / (a + x)                                  = %s", buf);
	ftoa(1.0 - a / (a + x), buf, 6);				printf("\n1.0 - a / (a + x)                            = %s", buf);
	ftoa(1.0 / ( a / x + 1), buf, 6);				printf("\n1.0 / ( a / x + 1)                           = %s", buf);
	ftoa((1.0 - (a + 0.1 * x) / (a + x)) / 0.9, buf, 6);		printf("\n(1.0 - (a + 0.1 * x) / (a + x)) / 0.9        = %s", buf);
	ftoa(a / ( a + x), buf, 6);					printf("\na / ( a + x)                                 = %s", buf);
	ftoa(1.0 - x / (a + x), buf, 6);				printf("\n1.0 - x / (a + x)                            = %s", buf);
	ftoa(1.0 / (1.0 + x / a), buf, 6);				printf("\n1.0 / (1 + x / a)                            = %s", buf);
	ftoa((a + x / 10.0) / (a + x) - 0.1 * x / (a + x), buf, 6);	printf("\n(a + x / 10.0) / (a + x) - 0.1 * x / (a + x) = %s", buf);
	ftoa(log((a + x) / a), buf, 6);					printf("\nlog((a + x) / a))                            = %s", buf);
	ftoa(log(a + x) - log(a), buf, 6);				printf("\nlog(a + x) - log(a)                          = %s", buf);
	ftoa(log(1.0 + x / a), buf, 6);					printf("\nlog(1.0 + x / a)                             = %s", buf);
	ftoa(-log(1.0 - x / (a + x)), buf, 6);				printf("\n-log(1.0 - x / (a + x))                      = %s", buf);
	printf("\n");
}

void testfp(void){
	int i;
	double a, x;
	char buf[30];

	ftoa(epsilon(), buf, 6);
	printf("\nmachine epsilon: %s", buf);

	a = 3.0 / 4.0;
	for (i = -10; i > -13; i--){
		x = pow(3.0, i);
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
	double e;
	int8_t buf[30];

	e = 0;
	for (i = 0; i < 13; i++){
		e += 1.0 / (double)fac(i);

		ftoa(e, buf, 6);
		printf("[%d] - e = %s\n", i, buf);
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
