#include <csignal>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	volatile std::sig_atomic_t gSignalStatus{};
}

extern "C" void signalHandler(int sig)
{
	gSignalStatus = sig;
}

int readFile(std::string_view fileName, std::string& out)
{
	std::ifstream inp{static_cast<std::string>(fileName)};
	if (!inp)
	{
		std::cout << "Failed to open file: " << fileName << '\n';
		return 1;
	}

	std::stringstream filestr;
	while (std::getline(inp, out))
		filestr << out;
	out = filestr.str();
	std::size_t stack{};
	for (const auto ch : out)
	{
		if (ch == '[')
			++stack;
		else if (ch == ']')
			--stack;
	}
	if (stack != 0)
	{
		std::cerr << "Invalid input: unmatched brackets\n";
		return 3;
	}

	return 0;
}

int getInput(std::string& to)
{
	std::string str;
	if (!std::cin.eof())
	{
		std::cout << "\r\x1b[1;34;32m>\x1b[0m ";
		std::getline(std::cin, str);
		to += str;
	}
	else
		return 5;

	return 0;
}

int longSkip(std::string& code, std::size_t& index, bool& printed)
{
	std::size_t stack{};
	while (gSignalStatus == 0)
	{
		if (index >= code.size())
		{
			if (printed)
				std::cout << '\n';
			printed = false;
			int status{getInput(code)};
			if (status) return status;
		}
		if (index < code.size())
		{
			switch (code[index])
			{
				case '[':
					++stack;
					break;
				case ']':
					--stack;
					if (stack == 0)
						return 0;
					break;
			}
			++index;
		}
	}
	return 0;
}

int runCode(std::string& code, bool interactiveMode)
{
	if (interactiveMode)
		std::cout << "Brainfuck interactive console (experimental)\n";

	std::vector<unsigned char> cells(30'000, 0);
	int currentCell{};

	std::size_t i{};

	bool printed{};

	while ((i < code.size() || interactiveMode) 
			&& gSignalStatus == 0)
	{
		if (interactiveMode && i >= code.size())
		{
			if (printed)
				std::cout << '\n';
			printed = false;
			int status{getInput(code)};
			if (status) return status;
		}
		if (i < code.size())
		{
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
						cells[currentCell] =
							static_cast<unsigned char>(std::cin.get());
						break;
					case '[':
						if (cells[currentCell] == 0)
						{ // move past the corresponding bracket
							std::size_t stack{};
							std::size_t localIndex{i};
							for (++localIndex; !(code[localIndex] == ']'
										&& stack == 0)
									&& localIndex < code.size();
									++localIndex)
							{
								if (code[localIndex] == '[') ++stack;
								else if (code[localIndex] == ']') --stack;
							}
							if (stack == 0 && code[localIndex] == ']')
								i = localIndex;
							else if (interactiveMode)
								longSkip(code, i, printed);
							else
								return 2;
						}
						break;
					case ']':
						if (cells[currentCell] != 0)
						{ // move past the corresponding bracket
							std::size_t stack{};
							std::size_t localIndex{i};
							for (--localIndex; !(code[localIndex] == '['
										&& stack == 0)
									&& localIndex != 0;
									--localIndex)
							{
								if (code[localIndex] == ']') ++stack;
								else if (code[localIndex] == '[') --stack;
							}
							if (stack == 0 && code[localIndex] == '[')
								i = localIndex;
							else if (!interactiveMode)
								return 2;
						}
						break;
				}
			++i;
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	std::signal(SIGINT, signalHandler);
	bool interactiveMode{argc < 2 ? true : false};
	std::string code;

	if (!interactiveMode)
	{
		int status{readFile(argv[1], code)};
		if (status) return status; // if failed
	}

	return runCode(code, interactiveMode);
}
