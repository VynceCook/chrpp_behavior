/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the Apache License, Version 2.0.
 *
 *  Copyright:
 *     2016, Vincent Barichard <Vincent.Barichard@univ-angers.fr>
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 */

#if !defined( __cpp_exceptions )
#include <iostream>
int main()
{
   std::cerr << "Exception support required, example unavailable." << std::endl;
   return 1;
}
#else

#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>

#include <visitor/program.hh>

#include <constants.hh>
#include <options.hpp>

#include <chrppc_parse_build.hpp>

int main( int argc, const char** argv )
{
	Options options = parse_options(argc, argv, {
			{ "version", "", false, "Show chrppc version."},
			{ "chr_prg_names_only", "pno", false, "Print, to the standard output, the program names of the input source code on the standard output and exit. Ignore all other non relevent parameters (warning, optimization, ...)."},
			{ "chr_output_files_only", "ofo", false, "Print, to the standard output, the new files that will be created based on the input source code. Ignore all other non relevent parameters (warning, optimization, ...)."},
			{ "trace", "t", false, "Enable trace output of CHR actions."},
			{ "stdin", "sin", false, "Listen standard input for incoming CHR statements instead of analyzing in a source file."},
			{ "stdout", "sout", false, "Output all CHR generated code to standard output."},
			{ "output_dir", "o", true, "Output directory for all CHR generated file (only if no --stdout set)."},
			{ "enable-warning_unused_rule", "", false, "Enable warning about unused ruled detection (default)."},
			{ "disable-warning_unused_rule", "wnu", false, "Disable warning about about unused ruled detection."},
			{ "enable-never_stored", "ns", false, "Enable never stored optimization (default)."},
			{ "disable-never_stored", "", false, "Disable never stored optimization."},
			{ "enable-head_reorder", "", false, "Enable head reorder optimization (default)."},
			{ "disable-head_reorder", "", false, "Disable head reorder optimization."},
			{ "enable-guard_reorder", "", false, "Enable guard reorder optimization (default)."},
			{ "disable-guard_reorder", "", false, "Disable guard reorder optimization."},
			{ "enable-occurrences_reorder", "", false, "Enable occurrences reorder optimization (default)."},
			{ "disable-occurrences_reorder", "", false, "Disable occurrences reorder optimization."},
			{ "enable-constraint_store_index", "csi", false, "Enable the use of an indexing data structure for managing constraint store (default)."},
			{ "disable-constraint_store_index", "", false, "Disable the use of an indexing data structure for managing constraint store."},
			{ "enable-line_error", "le", false, "Enable friendly line error in chrpp source file (default)."},
			{ "disable-line_error", "", false, "Disable friendly line error in chrpp source file."},
			{ "", "", false, "File name to parse."}
	});

	Options_values value_output_dir;
	if (has_option("version", options))
	{
		std::cout << chr::compiler::Compiler_options::CHRPPC_MAJOR << "." << chr::compiler::Compiler_options::CHRPPC_MINOR << std::endl;
		return 0;
	}
	if (has_option("trace", options))
		chr::compiler::Compiler_options::TRACE = true;
	if (has_option("stdin", options))
		chr::compiler::Compiler_options::STDIN = true;
	if (has_option("stdout", options))
		chr::compiler::Compiler_options::STDOUT = true;
	if (has_option("output_dir", options, value_output_dir))
	{
		std::string o_dir = value_output_dir[0].str();
		if ((!o_dir.empty()) && (o_dir[o_dir.size() - 1] != '/'))
			chr::compiler::Compiler_options::OUTPUT_DIR = o_dir + "/";
		else
			chr::compiler::Compiler_options::OUTPUT_DIR = o_dir;
	}
	if (has_option("disable-never_stored", options))
		chr::compiler::Compiler_options::NEVER_STORED = false;
	if (has_option("enable-never_stored", options))
		chr::compiler::Compiler_options::NEVER_STORED = true;
	if (has_option("disable-head_reorder", options))
		chr::compiler::Compiler_options::HEAD_REORDER = false;
	if (has_option("enable-head_reorder", options))
		chr::compiler::Compiler_options::HEAD_REORDER = true;
	if (has_option("disable-guard_reorder", options))
		chr::compiler::Compiler_options::GUARD_REORDER = false;
	if (has_option("enable-guard_reorder", options))
		chr::compiler::Compiler_options::GUARD_REORDER = true;
	if (has_option("disable-occurrences_reorder", options))
		chr::compiler::Compiler_options::OCCURRENCES_REORDER = false;
	if (has_option("enable-occurrences_reorder", options))
		chr::compiler::Compiler_options::OCCURRENCES_REORDER = true;
	if (has_option("disable-constraint_store_index", options))
		chr::compiler::Compiler_options::CONSTRAINT_STORE_INDEX = false;
	if (has_option("enable-constraint_store_index", options))
		chr::compiler::Compiler_options::CONSTRAINT_STORE_INDEX = true;
	if (has_option("disable-line_error", options))
		chr::compiler::Compiler_options::LINE_ERROR = false;
	if (has_option("enable-line_error", options))
		chr::compiler::Compiler_options::LINE_ERROR = true;
	if (has_option("disable-warning_unused_rule", options))
		chr::compiler::Compiler_options::WARNING_UNUSED_RULE = false;
	if (has_option("enable-warning_unused_rule", options))
		chr::compiler::Compiler_options::WARNING_UNUSED_RULE = true;

	// Disable line_error if we read stdin or write to stdout
	if (chr::compiler::Compiler_options::STDIN || chr::compiler::Compiler_options::STDOUT)
		chr::compiler::Compiler_options::LINE_ERROR = false;


	Options_values file_names;
	// -----------------------------------------------------------------
	// If we only have to print CHR program names or new created file names
	if (has_option("chr_prg_names_only", options) || has_option("chr_output_files_only", options))
	{
		std::vector< std::string > prg_names;
		std::vector< std::string > prg_file_names;
		if (has_option("", options, file_names))
		{
			std::string input_file = file_names[0].str();
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
			std::string output_file_base = chr::compiler::Compiler_options::OUTPUT_DIR + input_file_name;
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
					prg_names.push_back(pb_name);
					++n;
				}
			}
			ifs_input_file.close();
		} else {
			if (!chr::compiler::Compiler_options::STDIN)
			{
				std::cerr << "error: if no source file is given, the option --stdin is expected" << std::endl;
				return 1;
			}
		}

		if (prg_names.size() > 0)
		{
			if (has_option("chr_prg_names_only", options))
			{
				for (unsigned int i=0; i < prg_names.size(); ++i)
					std::cout << (i==0?"":" ") << prg_names[i];
				std::cout << std::endl;
			}
			if (has_option("chr_output_files_only", options))
			{
				for (unsigned int i=0; i < prg_file_names.size(); ++i)
					std::cout << prg_file_names[i];
			}
		}

		return 0;
	}

	// -----------------------------------------------------------------
	// Parse input
	std::vector< chr::compiler::parser::ast_builder::AstProgramBuilder > chr_prg_results;
	int ret = 0;
	if (has_option("", options, file_names))
	{
		std::string input_file = file_names[0].str();
		if (!std::filesystem::exists( input_file ))
		{
			std::cerr << R"_STR(error: unknown argument (or can't open file): ")_STR" << input_file << R"_STR(")_STR" << std::endl;
				return 1;
		}

		TAO_PEGTL_NAMESPACE::file_input in(input_file);
		ret = chr::compiler::parse_chr_input(in, chr_prg_results, input_file);
	} else {
		if (!chr::compiler::Compiler_options::STDIN)
		{
			std::cerr << "error: if no source file is given, the option --stdin is expected" << std::endl;
			return 1;
		}
	
		std::string str_content;
		std::string str_line;
		while (!std::getline(std::cin, str_line).fail())
			str_content += str_line;
		TAO_PEGTL_NAMESPACE::string_input in( str_content, "std::cin" );
		ret = chr::compiler::parse_chr_input(in, chr_prg_results);
	}
	// If parsing failed, stop here
	if (ret != 0) return ret;

	// Generate CPP code
	if (chr::compiler::Compiler_options::STDOUT)
	{
		// Output to STDOUT
		unsigned int n_chr_prg = 0;
		for (auto& prg_res : chr_prg_results)
		{
			auto& chr_prg = prg_res.prg;
	
			// Print infos
			chr::compiler::visitor::ProgramPrint vp4;
			std::cout << "/*\n";
			std::cout << "From line " << prg_res._start_line_number << " to " << prg_res._end_line_number << "\n";
			std::cout << prg_res._str_dependency_graph << "\n";
			std::cout << vp4.string_from(chr_prg) << "\n";
			chr::compiler::ast::ChrProgram chr_prg_dup(chr_prg);
			chr::compiler::visitor::ProgramAbstractCode vp5(std::cout);
			vp5.generate(chr_prg_dup);
			std::cout << "*/\n";

			// Put header in output header file, only when output is file and not
			// stdin. Header start when the last CHR program ends or from the start
			// of input file if it is the first CHR program.
			if (!chr::compiler::Compiler_options::STDIN)
			{
				std::cout << "//----------------------- START INCLUDE HEADER ---------------------\n";
				std::string input_file = file_names[0].str();
				std::ifstream ifs_input_file;
				ifs_input_file.open(input_file);
				if (!ifs_input_file.is_open())
				{
					std::cerr << R"_STR(Can't open file ")_STR" << input_file<< R"_STR(")_STR" << std::endl;
					return 1;
				}
				// Start from the first line or the last ligne of previous CHR
				// program.
				bool first_print = true;
				unsigned int start_line = (n_chr_prg==0)?1:(chr_prg_results[n_chr_prg-1]._end_line_number+1);
				unsigned int n_line = 1;
				int nb_open_comments = 0;
				std::string str_line;
				while ((n_line < prg_res._start_line_number) && !std::getline(ifs_input_file, str_line).fail())
				{
					assert(nb_open_comments >= 0);
					if (n_line >= start_line)
					{
						if (first_print && (nb_open_comments > 0))
							std::cout << "/*\n";
						first_print = false;
						std::cout << str_line << "\n";
					}

					// Update the number of open comments
					for (int j=0; j < ((int)str_line.length() - 1); ++j)
						if ((str_line[j] == '/') && (str_line[j+1] == '*')) ++nb_open_comments;
						else if ((str_line[j] == '*') && (str_line[j+1] == '/')) --nb_open_comments;
					++n_line;
				}
				if (nb_open_comments > 0)
					std::cout << "*/\n";
				ifs_input_file.close();
				std::cout << "//----------------------- END INCLUDE HEADER ---------------------\n";
			}

			// Generate output program
			chr::compiler::visitor::ProgramCppCode vp6(std::cout);
			vp6.generate(chr_prg);
			++n_chr_prg;
		}

		// Put header in output header file, only when output is file and not
		// stdin. Header start when the last CHR program ends or from the start
		// of input file if it is the first CHR program.
		if (!chr::compiler::Compiler_options::STDIN)
		{
			std::cout << "//----------------------- START INCLUDE HEADER ---------------------\n";
			std::string input_file = file_names[0].str();
			std::ifstream ifs_input_file;
			ifs_input_file.open(input_file);
			if (!ifs_input_file.is_open())
			{
				std::cerr << R"_STR(Can't open file ")_STR" << input_file<< R"_STR(")_STR" << std::endl;
				return 1;
			}
			// Start from the first line or the last ligne of previous CHR
			// program.
			bool first_print = true;
			unsigned int start_line = (n_chr_prg==0)?1:(chr_prg_results[n_chr_prg-1]._end_line_number+1);
			unsigned int n_line = 1;
			int nb_open_comments = 0;
			std::string str_line;
			while (!std::getline(ifs_input_file, str_line).fail())
			{
				assert(nb_open_comments >= 0);
				if (n_line >= start_line)
				{
					if (first_print && (nb_open_comments > 0))
						std::cout << "/*\n";
					first_print = false;
					std::cout << str_line << "\n";
				}

				// Update the number of open comments
				for (int j=0; j < ((int)str_line.length() - 1); ++j)
					if ((str_line[j] == '/') && (str_line[j+1] == '*')) ++nb_open_comments;
					else if ((str_line[j] == '*') && (str_line[j+1] == '/')) --nb_open_comments;
				++n_line;
			}
			if (nb_open_comments > 0)
				std::cout << "*/\n";
			ifs_input_file.close();
			std::cout << "//----------------------- END INCLUDE HEADER ---------------------\n";
		}
	} else {
		// Output to files
		// Extract parts of input file name
		std::string input_file = file_names[0].str();
		std::string input_file_base = input_file.substr(0,input_file.find_last_of('.'));
		std::string input_file_name = input_file_base.substr(input_file_base.find_last_of('/') + 1);
		std::string input_file_ext = input_file.substr(input_file.find_last_of('.'));

		// -----------------------------------------------------------------
		// Compute output file parts
		std::string output_file_base = chr::compiler::Compiler_options::OUTPUT_DIR + input_file_name;
		std::string output_file_ext = "_chr" + input_file_ext;
		if (input_file_ext == ".chrpp")
			output_file_ext = "_chr.cpp";

		// -----------------------------------------------------------------
		// Generate output for each program of input file
		std::vector< std::string > header_include_lines;
		unsigned int n_chr_prg = 0;
		for (auto& prg_res : chr_prg_results)
		{
			auto& chr_prg = prg_res.prg;

			std::ofstream output_header_file;
			std::ofstream output_program_file;

			header_include_lines.push_back(output_file_base + std::string(chr_prg.name()) + ".hh");
			output_header_file.open(header_include_lines.back());
			if (!output_header_file.is_open())
			{
				std::cerr << R"_STR(Can't create file ")_STR" << output_file_base << chr_prg.name() << R"_STR(.hh")_STR" << std::endl;
				return 1;
			}
			std::string output_program_file_name = output_file_base + std::string(chr_prg.name()) + (chr_prg.template_parameters().empty()?".cpp":".hpp");
			output_program_file.open(output_program_file_name);
			if (!output_program_file.is_open())
			{
				std::cerr << R"_STR(Can't create file ")_STR" << output_program_file_name << R"_STR(")_STR" << std::endl;
				return 1;
			}

			chr::compiler::ast::ChrProgram chr_prg_dup(chr_prg);
			// -----------------------------------------------------------------
			// Header file
			// Print infos
			{
				if (!chr_prg.template_parameters().empty())
					chr_prg.set_include_hpp_file(output_program_file_name);

				chr::compiler::visitor::ProgramPrint vp4;
				if (!chr::compiler::Compiler_options::STDIN)
				{
					output_header_file << "#ifndef " << chr_prg.name() << "__GUARD\n";
				 	output_header_file << "#define " << chr_prg.name() << "__GUARD\n";
				}
				output_header_file << "/*\n";
				output_header_file << "From line " << prg_res._start_line_number << " to " << prg_res._end_line_number << "\n";
				output_header_file << prg_res._str_dependency_graph << "\n";
				output_header_file << vp4.string_from(chr_prg) << "\n";
				chr::compiler::visitor::ProgramAbstractCode vp5(output_header_file);
				vp5.generate(chr_prg_dup,true,false);
				output_header_file << "*/\n";

				for (unsigned int i=0; i < (header_include_lines.size() - 1); ++i)
					output_header_file << "#include <" << header_include_lines[i] << ">\n";

				// Put header in output header file, only when output is file and not
				// stdin. Header start when the last CHR program ends or from the start
				// of input file if it is the first CHR program.
				if (!chr::compiler::Compiler_options::STDIN)
				{
					output_header_file << "//----------------------- START INCLUDE HEADER ---------------------\n";
					std::ifstream ifs_input_file;
					ifs_input_file.open(input_file);
					if (!ifs_input_file.is_open())
					{
						std::cerr << R"_STR(Can't open file ")_STR" << input_file<< R"_STR(")_STR" << std::endl;
						return 1;
					}
					// Start from the first line or the last ligne of previous CHR
					// program.
					bool first_print = true;
					unsigned int start_line = (n_chr_prg==0)?1:(chr_prg_results[n_chr_prg-1]._end_line_number+1);
					unsigned int n_line = 1;
					int nb_open_comments = 0;
					std::string str_line;
					while ((n_line < prg_res._start_line_number) && !std::getline(ifs_input_file, str_line).fail())
					{
						assert(nb_open_comments >= 0);
						if (n_line >= start_line)
						{
							if (first_print && (nb_open_comments > 0))
								output_header_file << "/*\n";
							first_print = false;
							output_header_file << str_line << "\n";
						}

						// Update the number of open comments
						for (int j=0; j < ((int)str_line.length() - 1); ++j)
							if ((str_line[j] == '/') && (str_line[j+1] == '*')) ++nb_open_comments;
							else if ((str_line[j] == '*') && (str_line[j+1] == '/')) --nb_open_comments;
						++n_line;
					}
					if (nb_open_comments > 0)
						output_header_file << "*/\n";
					ifs_input_file.close();
					output_header_file << "//----------------------- END INCLUDE HEADER ---------------------\n";
				}

				// Generate output program
				chr::compiler::visitor::ProgramCppCode vp6(output_header_file);
				vp6.generate(chr_prg,true,false);
				if (!chr::compiler::Compiler_options::STDIN)
					output_header_file << "#endif // " << chr_prg.name() << "__GUARD\n";
			}

			// -----------------------------------------------------------------
			// Cpp code file (rules)
			// Print infos
			{
				chr::compiler::visitor::ProgramPrint vp4;
				output_program_file << "/*\n";
				output_program_file << "From line " << prg_res._start_line_number << " to " << prg_res._end_line_number << "\n";
				output_program_file << vp4.string_from(chr_prg) << "\n";
				chr::compiler::visitor::ProgramAbstractCode vp5(output_program_file);
				vp5.generate(chr_prg_dup,false,true);
				output_program_file << "*/\n";

				// Generate output program
				output_program_file << "#include <" << header_include_lines.back() << ">\n";
				chr::compiler::visitor::ProgramCppCode vp6(output_program_file);
				vp6.generate(chr_prg,false,true);
			}
			output_header_file.close();
			output_program_file.close();
			++n_chr_prg;
		}

		// -----------------------------------------------------------------
		// Strip core file
		// We do not generate strip file if incoming input came from standard input
		if (!chr::compiler::Compiler_options::STDIN)
		{
			std::ofstream output_main_file;
			output_main_file.open(output_file_base + output_file_ext);
			if (!output_main_file.is_open())
			{
				std::cerr << R"_STR(Can't create file ")_STR" << output_file_base << output_file_ext << R"_STR(")_STR" << std::endl;
				return 1;
			}

			std::ifstream ifs_input_file;
			ifs_input_file.open(input_file);
			if (!ifs_input_file.is_open())
			{
				std::cerr << R"_STR(Can't open file ")_STR" << input_file<< R"_STR(")_STR" << std::endl;
				return 1;
			}

			// Output include of CHR header files
			unsigned int j = 0;
			for (const auto& header : header_include_lines)
			{
				output_main_file << "#include <" << header << ">\n";
				if (chr::compiler::Compiler_options::LINE_ERROR)
					output_main_file <<  "#line " << j << R"_STR( ")_STR" << input_file <<  R"_STR(")_STR" << "\n";
				++j;
			}

			assert(chr_prg_results.size() > 0);
			bool first = true;
			unsigned int n_line = 1;
			int nb_open_comments = 0;
			// Strip from the start to the end of last CHR program
			unsigned int _start_line = chr_prg_results.back()._end_line_number + 1;
			std::string str_line;
			while (!std::getline(ifs_input_file, str_line).fail())
			{
				if (n_line >= _start_line)
				{
					if (first && (nb_open_comments > 0))
						output_main_file << "/*\n";
					first = false;
					output_main_file << str_line << "\n";
				}

				// Update the number of open comments
				for (int j=0; j < ((int)str_line.length() - 1); ++j)
					if ((str_line[j] == '/') && (str_line[j+1] == '*')) ++nb_open_comments;
					else if ((str_line[j] == '*') && (str_line[j+1] == '/')) --nb_open_comments;

				++n_line;
			}

			ifs_input_file.close();
			output_main_file.close();
		}
	}
	return 0;
}

#endif
