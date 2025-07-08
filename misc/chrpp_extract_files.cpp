#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>

int main( int argc, const char** argv )
{
	if (argc != 2)
	{
		std::cerr << "Missing argument, a file name must be provided" << std::endl;
		return 1;
	}

	std::vector< std::string > prg_file_names;
	std::string input_file(argv[1]);
	std::ifstream ifs_input_file;
	ifs_input_file.open(input_file);
	if (!ifs_input_file.is_open())
	{
		std::cerr << R"_STR(error: unknown argument (or can't open file): ")_STR" << input_file << R"_STR(")_STR" << std::endl;
		return 1;
	}

	// Computes output file names
	std::string input_file_base = input_file.substr(0,input_file.find_last_of('.'));
	std::string input_file_name = input_file_base.substr(input_file_base.find_last_of('/') + 1);
	std::string input_file_ext = input_file.substr(input_file.find_last_of('.'));
	std::string output_file_base = input_file_name;
	std::string output_file_ext = "_chr" + input_file_ext;
	if (input_file_ext == ".chrpp")
		output_file_ext = "_chr.cpp";
	prg_file_names.push_back(std::string(output_file_base + output_file_ext));

	// Search for <chr name=".."" regex
	std::regex regex_expr1(R"(<chr[ >])", std::regex::extended|std::regex::icase);
	std::regex regex_expr2(R"_STR(<chr .*name="([^"]*)")_STR", std::regex::extended|std::regex::icase);
	std::regex regex_expr3(R"_STR(<chr .*template_parameters="[^"]+")_STR", std::regex::extended|std::regex::icase);
	std::smatch regex_match;
	std::string str_line;
	unsigned int n = 0;
	while (!std::getline(ifs_input_file, str_line).fail())
	{
		if (std::regex_search(str_line, regex_expr1))
		{
			std::string pb_name("_CHR_PRG_" + std::to_string(n));
			if (std::regex_search(str_line, regex_match, regex_expr2))
				pb_name = regex_match[1];

			std::string tmp;
			tmp += ";" + output_file_base + pb_name + ".hh";
			if (std::regex_search(str_line, regex_expr3))
				tmp += ";" + output_file_base + pb_name + ".hpp";
			else
				tmp += ";" + output_file_base + pb_name + ".cpp";
			prg_file_names.push_back(tmp);
			++n;
		}
	}
	ifs_input_file.close();

	for (unsigned int i=0; i < prg_file_names.size(); ++i)
		std::cout << prg_file_names[i];
	return 0;
}

