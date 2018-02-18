//
// First laboratory work on system and software engineering (var. 13)
// Made by Ivan Kalinin, IVBO-01-16
//
// This program was tested on Visual Studio 2017 Community with C++ SDK 10.0.16299.0
//

#include <iterator>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
	int A = -10;
	int B = 5;

	// task input
	std::string input;
	std::getline(std::cin, input);
	std::stringstream input_stream(input);

	std::vector<int> values(
		(std::istream_iterator<int>(input_stream)),
		(std::istream_iterator<int>()));

	// task solution
	int summ = 0;
	for (size_t i = 0; i < values.size(); ++i) {
		if (values[i] > A && values[i] < B) {
			summ += values[i];
		}
	}

	std::cout << summ;

	return 0;
}
