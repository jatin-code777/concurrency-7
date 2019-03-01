#pragma once

#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <functional>
#include "SearchStrategy.h"
#include <mutex>
#include <sstream>


	class regex_search : public SearchStrategy
	{
		
	public:
		
		void pre_process(char* pattern, bool ignore_case, bool single_file , int flags)
		{
			using namespace std::regex_constants;
			auto flag_mask = nosubs | optimize ; //Specify grammar
			if(ignore_case) flag_mask |= icase;
			try {
				expr.assign(pattern, flag_mask);
			} catch (std::regex_error& e) {
				std::cerr << "regex_error : " << e.what() << '\n';
				throw e;	// What to do now ??
			}

			print_file_name = !single_file;
			print_line_num = (flags == 4);
			print_matched_file = (flags == 1);

			if(flags == 0 or flags == 4) strategy = [this](std::ifstream& input,std::string s) {this->normal(input,s);};
			else if(flags == 3) strategy = [this](std::ifstream& input,std::string s) {this->line_count_only(input,s);};
			else strategy = [this](std::ifstream& input,std::string s) {this->file_name_only(input,s);};

		}

		void search(int id, std::string fpath) {
			std::ifstream input(fpath); //RAII acquire file
			if(not input.is_open()) {
				print_mutex.lock();
				printf("grape: Unable to open: %s\n",fpath.data());
				print_mutex.unlock();
				return;
			}
			strategy(input,fpath);
			input.close();
		}

		void file_name_only(std::ifstream& input, const std::string &path) {
			std::string line;
			while(std::getline(input,line)) {
				if(std::regex_search(line,expr)) {
					if(print_matched_file) {
						print_mutex.lock();
						std::cout<<COLOR_PURPLE<<path<<COLOR_RESET;
						//printf(COLOR_PURPLE "%s\n" COLOR_RESET, path.data());; 
						print_mutex.unlock(); }
					return;
				}
			}
			if(!print_matched_file) {
				print_mutex.lock();
				std::cout<<COLOR_PURPLE<<path<<COLOR_RESET;
				//printf(COLOR_PURPLE "%s\n" COLOR_RESET, path.data());; 
				print_mutex.unlock();
			}
		}

		void line_count_only(std::ifstream& input, const std::string &path) {

			int lines_matched = 0;
			std::string line;
			while(std::getline(input,line)) {
				if(std::regex_search(line,expr))
					++lines_matched;
			}
			print_mutex.lock();
			if(print_file_name) std::cout<<COLOR_PURPLE<<path<<COLOR_RESET<<COLOR_CYAN<<":"<<COLOR_RESET;//printf(COLOR_PURPLE "%s:" COLOR_RESET, path.data());; 
			printf("%d\n",lines_matched);
			print_mutex.unlock();
		}

		void normal(std::ifstream& input, const std::string &path)
		{
			int line_num = 1;
			std::string line;
			std::stringstream ss;
			while(std::getline(input,line)) {
				output_matches(line,line_num,path,ss);
				++line_num;
			}
			std::lock_guard<std::mutex> lock(print_mutex);
			printf("%s",ss.str().data());
		}

		void output_matches(std::string &line,int line_num, const std::string &path, std::stringstream& ss)
		{
			try {
				std::sregex_iterator next(line.begin(), line.end(), expr);
				std::sregex_iterator end;
				if(next!=end)
				{
					size_t cur_pos = 0;

					if(print_file_name) ss<<COLOR_PURPLE<<path.data()<<COLOR_RESET<<COLOR_CYAN<<":"<<COLOR_RESET;//printf(COLOR_PURPLE "%s:" COLOR_RESET, path.data());
					if(print_line_num)  ss<<COLOR_RED<<line_num<<COLOR_RESET<<COLOR_CYAN<<":"<<COLOR_RESET;//printf(COLOR_RED	"%d:" COLOR_RESET, line_num);

					while (next != end) {
						std::smatch match = *next;
						// printf("%.*s",int(match.position()-cur_pos), line.data()+cur_pos);
						ss.rdbuf()->sputn(line.data()+cur_pos,int(match.position()-cur_pos));
						ss<<COLOR_RED_BOLD<<match.str()<<COLOR_RESET;//printf(COLOR_RED_BOLD "%s" COLOR_RESET, match.str().data());
						cur_pos = match.position()+match.length();
						next++;
					}
					ss<<line.data()+cur_pos<<"\n";
					//printf("%s\n",line.data()+cur_pos);
				}
			} catch (std::regex_error& e) {
				std::cerr<<e.what()<<"\n";
			// Syntax error in the regular expression
			}
		}

	private:
		bool print_line_num , print_file_name, print_matched_file;
		std::regex expr;
		std::function <void(std::ifstream&,std::string)> strategy;
		std::mutex print_mutex;
		
	};