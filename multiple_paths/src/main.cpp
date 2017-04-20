#include "flow.h"
#include <iostream>
#include <chrono>

int main(int argc, char* argv[])
{
	auto const start = std::chrono::high_resolution_clock::now();
	if (runiFRoC( argc,  argv) == 1)
	{
		std::cout << "iFRoC finished successfully " << std::endl;
		auto const end = std::chrono::high_resolution_clock::now();

		auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "*****************************************************" << std::endl;
		std::cout << "iFRoC finished in "  << (delta_time.count()) / (1000.0) << " seconds " << std::endl;
		std::cout << "*****************************************************" << std::endl;
	}
	else
	{
		std::cout << "iFRoC failed " << std::endl;
	}
}


