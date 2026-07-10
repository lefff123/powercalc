#include <iostream>
#include "matrix.h"

int main()
{
    std::cout << "Hello from PowerCalc!" << std::endl;
	Matrix a(2,2,1);
	a.print();
    std::cout << "Matrix created successfully!" << std::endl;

    return 0;
}