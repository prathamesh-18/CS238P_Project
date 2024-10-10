#include <math.h>
#include <stdio.h>
double sigmoid(double x) {
return 1.0 / (1.0 + exp(-x));
}
double evaluate(void) {
double t1 = 2.000000;

 double sigmoid_value = sigmoid(t1);
printf("Sigmoid Value of the Result of your given expression is:");
printf("%f\n", sigmoid_value);
return t1;
}
