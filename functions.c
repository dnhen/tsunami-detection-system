#include <stdio.h>
#include <stdlib.h>
#include "functions.h"

// Generate float values between minVal and maxVal
float generateFloatValue(int minVal, int maxVal){
    float scale = rand() / (float) RAND_MAX; // [0, 1.0];
    return minVal + scale * ( maxVal - minVal); // [min, max];
}

// Generate random int between two values
int generateInt(int minVal, int maxVal){
	return (rand() % (maxVal - minVal + 1)) + minVal;
}