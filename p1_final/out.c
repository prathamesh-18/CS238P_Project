#include <stdio.h>
double* evaluate(void) {
double t1 = 22.000000;
double t3 = 7.000000;
double t2 = t3 ? (t1 / t3) : 0.0;
double t6 = 3.000000;
double t5 = - t6;
double t4 = t2 - t5;
static double values[2];

 double expression_value = t4;
values[0]=t4;
printf("Value of the given expression is: ");
printf("%f\n", expression_value);
double (*sigmoid)(double) = (double (*)(double))0x100605a30;
values[1]= sigmoid(4);
return values;
}
