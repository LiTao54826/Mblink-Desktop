#include <iostream>
#include <cstdio>

int main() {
    printf("Hello from printf!\n");
    fflush(stdout);
    
    std::cout << "Hello from cout!" << std::endl;
    std::cout.flush();
    
    fprintf(stderr, "Hello from stderr!\n");
    fflush(stderr);
    
    return 0;
}

