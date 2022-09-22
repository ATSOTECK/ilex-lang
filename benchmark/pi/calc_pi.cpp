#include <iostream>
#include <cstdlib>
#include <chrono>

#define N_POINTS 10000000
#define N_REPEATS 10

double estimatePi(int numPoints) {
    double x, y, rSquared;
    int withinCircle = 0;
    
    for (int i = 0; i < numPoints; ++i) {
        x = (double)rand() / RAND_MAX;
        y = (double)rand() / RAND_MAX;
        
        rSquared = x * x + y * y;
        if (rSquared <= 1) {
            ++withinCircle;
        }
    }
    
    double pi = (double)withinCircle / N_POINTS * 4;
    return pi;
}

int main() {
    double avgTime = 0;
    
    srand(42);
    
    for (int i = 0; i < N_REPEATS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        double pi = estimatePi(N_POINTS);
        auto stop = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time = stop - start;
        avgTime += time.count();
        std::cout << "Pi is approx " << pi << " and took " << time.count() << " seconds to compute." << std::endl;
    }
    
    std::cout << "On average pi took " << avgTime / N_REPEATS << " seconds to compute." << std::endl;
    
    return 0;
}
