#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

namespace
{
	std::string* gCodePtr;
	std::vector<unsigned char>* gCellsPtr;
}

extern "C" void signalHandler(int)
{
	delete gCodePtr;
	delete gCellsPtr;
	std::cout << '\n';
	std::exit(0);
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
	out += filestr.str();
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
		if (isatty(fileno(stdin)))
			std::cout << "\r\x1b[1;34;32m>\x1b[0m ";
		std::getline(std::cin, str);
		to += str;
	}
	else
	{
		std::cout << '\n';
		return 5;
	}

	return 0;
}

int longSkip(std::string& code, std::size_t& index, bool& printed)
{
	std::size_t stack{};
	while (true)
	{
		if (index >= code.size())
		{
			if (printed && isatty(fileno(stdin)))
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
	if (interactiveMode && isatty(fileno(stdin)))
		std::cout << "Brainfuck interactive console\n";

	gCellsPtr = new std::vector<unsigned char>(30'000, 0);
	std::vector<unsigned char>& cells{*gCellsPtr};
	int currentCell{};

	std::size_t i{};

	bool printed{};

	while (i < code.size() || interactiveMode)
	{
		if (interactiveMode && i >= code.size())
		{
			if (printed && isatty(fileno(stdin)))
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

int processArgs(int argc, char* argv[], std::string& code)
{
	unsigned char flags{};

	/* Flags guide
	 * bit #
	 *
	 * 0: --help
	 * 1: -e, --exec
	 */

	int execPos{};

	for (int i{1}; i < argc; ++i)
	{
		if (argv[i][0] == '-' && argv[i][1] == '-') // long option
		{
			flags |= !std::strcmp(argv[i], "--help");
			{
				int exec{std::strcmp(argv[i], "--exec")};
				flags |= !exec << 1;
				if (!exec)
					execPos = i + 1;
			}
		}
		else if (argv [i][0] == '-')
			for (int j{1}; argv[i][j] != 0; ++j)
			{
				switch (argv[i][j])
				{
					case 'e':
						flags |= 1 << 1;
						execPos = i + 1;
						break;
					default:
						std::cout << "Invalid option: -" << argv[i][j] << '\n';
						return 4;
				}
			}
		else if (i != execPos)
			readFile(argv[i], code);
	}

	if (flags & 1)
	{
		std::cout << "Usage: " << argv[0] << " [OPTIONS] [FILES]\n\n"
					 "Options:\n"
					 "\t-e, --exec\t\texecute the next argument as code\n"
					 "\t--help\t\t\tshow this help\n";
		return 0;
	}
	if (flags & 1 << 1)
	{
		if (execPos < argc)
			code += argv[execPos];
		else
		{
			std::cout << "Option -e requires another argument\n";
			return 4;
		}
	}

	return 0;
}

int main(int argc, char* argv[])
{
	std::signal(SIGINT, signalHandler);
	bool interactiveMode{argc < 2 ? true : false};
	gCodePtr = new std::string;

	{
		int status{processArgs(argc, argv, *gCodePtr)};
		if (status != 0) return status;
	}

	int status{runCode(*gCodePtr, interactiveMode)};
	if (status == 5)
		status = 0;
	return status;
}
