#include <csignal>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	volatile std::sig_atomic_t gSignalStatus{};
}

extern "C" void signalHandler(int sig)
{
	gSignalStatus = sig;
}

int main(int argc, char* argv[])
{
	std::signal(SIGINT, signalHandler);
	bool interactiveMode{};
	if (argc < 2)
	{
		interactiveMode = true;
	}
	std::string code;

	if (!interactiveMode)
	{
		std::ifstream inp{argv[1]};
		if (!inp)
		{
			std::cout << "Failed to open file: " << argv[1] << '\n';
			return 1;
		}

		std::stringstream filestr;
		while (std::getline(inp, code))
			filestr << code;
		code = filestr.str();
	}

	std::vector<unsigned char> cells(30'000, 0);
	int currentCell{};

	if (interactiveMode)
		std::cout << "Brainfuck interactive console (experimental)\n";

	std::size_t i{};

	bool longSkipActive{};
	std::size_t longSkipStack{};

	bool printed{};

	while ((i < code.size() || interactiveMode) 
			&& gSignalStatus == 0)
	{
		if (interactiveMode && i >= code.size())
		{
			std::string str;
			if (printed)
				std::cout << '\n';
			std::cout << "\r\x1b[1;34;32m>\x1b[0m ";
			printed = false;
			std::getline(std::cin, str);
			if (std::cin.eof())
				break;
			code += str;
		}
		if (i < code.size())
		{
			if (!longSkipActive)
				switch (code[i])
				{
					case '+':
						++cells[currentCell];
						break;
					case '-':
						--cells[currentCell];
						break;
					case '>':
						++currentCell;
						break;
					case '<':
						--currentCell;
						break;
					case '.':
						std::cout << cells[currentCell];
						printed = true;
						break;
					case ',':
						std::cin >> cells[currentCell];
						break;
					case '[':
						if (cells[currentCell] == 0)
						{ // move past the corresponding bracket
							std::size_t stack{};
							std::ptrdiff_t localIndex{
								static_cast<std::ptrdiff_t>(i)
							};
							for (++localIndex; !(code[localIndex] == ']'
										&& stack == 0)
									&& localIndex < static_cast<std::ptrdiff_t>(code.size());
									++localIndex)
							{
								if (code[localIndex] == '[') ++stack;
								else if (code[localIndex] == ']') --stack;
							}
							if (stack == 0 && code[localIndex] == ']')
								i = localIndex;
							else if (interactiveMode)
								longSkipActive = true;
							else
								return 2;
						}
						break;
					case ']':
						if (cells[currentCell] != 0)
						{ // move past the corresponding bracket
							std::size_t stack{};
							std::ptrdiff_t localIndex{static_cast<std::ptrdiff_t>(i)};
							for (--localIndex; !(code[localIndex] == '['
										&& stack == 0)
									&& localIndex >= 0;
									--localIndex)
							{
								if (code[localIndex] == ']') ++stack;
								else if (code[localIndex] == '[') --stack;
							}
							if (stack == 0 && code[localIndex] == '[')
								i = localIndex;
							else
							{
								if (!interactiveMode)
									return 2;
							}
						}
						break;
				}
			else
			{
				switch (code[i])
				{
					case '[':
						++longSkipStack;
						break;
					case ']':
						if (longSkipStack == 0)
							longSkipActive = false;
						else
							--longSkipStack;
						break;
				}
			}
			++i;
		}
	}
}
