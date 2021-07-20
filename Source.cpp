/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

//#define GUI

#define NOMINMAX

#define VERSION "0.4.0" // MCPPPP version

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "utility.cpp"

#ifndef GUI
#include "fsb.cpp"
#include "vmt.cpp"
#include "cim.cpp"
#elif defined _WIN32
#include <Windows.h> // SetProcessDPIAware
#endif

int main(int argc, char* argv[])
{
#if defined _WIN32 && defined GUI
	SetProcessDPIAware(); // fix blurriness
#endif
	bool issetting = false, isvalue = false;
	std::string str, option, value, temp;
	std::stringstream ss;
	std::error_code ec;
#ifdef GUI
	Fl::get_system_colors();
	ui->show();
	Fl::wait();
#endif
	if (argc < 2) // skip file settings if there are command line settings
	{
		std::ifstream config("mcpppp.properties");
		if (config.fail() && argc < 2)
		{
			std::ofstream createconfig("mcpppp.properties");
			createconfig << "# MCPPPP will search folders for resource packs (such as your resourcepacks folder) and will edit the resource pack.\n# It won't touch anything but the necessary folders, and will skip the resourcepack if the folders already exist.\n# Enter a newline-seperated list of such folders" << std::endl;
			createconfig.close();
#ifdef GUI
			openhelp(nullptr, nullptr);
#else
			std::cerr << (dotimestamp ? timestamp() : "") << "Config file not found, look for mcpppp.properties" << std::endl;
			goto exit;
#endif
		}
		while (config.good()) // config file (mcpppp.properties)
		{
			getline(config, str);
			if (str.back() == '\r')
			{
				str.erase(str.end() - 1);
			}
			if (str[0] == '/' && str[1] == '/')
			{
				ss.clear();
				ss.str(str);
				ss >> temp;
				if (temp == "//set")
				{
					ss >> option;
					getline(ss, value);
					value.erase(value.begin());
					setting(option, value);
				}
			}
			else if (str[0] != '#' && str != "")
			{
				paths.insert(str);
			}
			else if (str.size() > 2)
			{
				if (str[1] == '!')
				{
					str.erase(str.begin(), str.begin() + 2);
					paths.erase(str);
#ifdef GUI
					deletedpaths.insert(str);
#endif
				}
			}
		}
		config.close();
	}
#ifndef GUI // gui doenst need command line options
	for (int i = 1; i < argc; i++) // function arguments
	{
		if (issetting) // if current argument is part of setting
		{
			if (isvalue) // if arg is value of setting
			{
				value = argv[i];
				while (value[value.size() - 1] != ';' && i < argc - 1)
				{
					i++;
					value += " " + std::string(argv[i]);
				}
				value.erase(value.end() - 1);
				issetting = false;
				isvalue = false;
				setting(option, value);
			}
			else // if arg is setting/option name
			{
				option = argv[i];
				isvalue = true;
			}
		}
		else
		{
			if (std::string(argv[i]) == "//set") // setting
			{
				issetting = true;
			}
			else if (argv[i][0] == '#') // comment
			{
				// not gonna be implementing #!path since it isn't necessary
				// in file it's necessary since gui has to remove certain paths
				temp = argv[i];
				while (temp[temp.size() - 1] != ';' && i < argc - 1)
				{
					i++;
					temp = argv[i];
				}
			}
			else // path
			{
				temp = argv[i];
				while (temp[temp.size() - 1] != ';' && i < argc - 1)
				{
					i++;
					temp += " " + std::string(argv[i]);
				}
				temp.erase(temp.end() - 1);
				paths.insert(temp);
			}
		}
	}
#endif

	out(5) << "MCPPPP " << VERSION
#ifdef GUI
		<< " (GUI)"
#else
		<< " (CLI)"
#endif
		<< std::endl << std::endl;
	out(5) << "pauseOnExit     " << (pauseonexit ? "true" : "false") << std::endl;
	out(5) << "log             " << logfile << std::endl;
	out(5) << "timestamp       " << (dotimestamp ? "true" : "false") << std::endl;
	out(5) << "autoDeleteTemp  " << (dotimestamp ? "true" : "false") << std::endl;
	out(5) << "outputLevel     " << outputlevel << std::endl;
	out(5) << "logLevel        " << loglevel << std::endl;
	out(5) << "autoReconvert   " << autoreconvert << std::endl;
	out(5) << std::endl << std::endl;

	if (std::filesystem::is_directory("mcpppp-temp"))
	{
		if (autodeletetemp)
		{
			out(4) << "Folder named \"mcpppp-temp\" found. Removing..." << std::endl;
			std::filesystem::remove_all("mcpppp-temp");
		}
		else
		{
			out(4) << "Folder named \"mcpppp-temp\" found. Please remove this folder." << std::endl;
			goto exit;
		}
	}
#ifdef GUI
	addpaths();
	updatepaths();
	dotimestamp = true;
	updatesettings();
#endif
	for (std::string path : paths)
	{
		if (!std::filesystem::is_directory(std::filesystem::u8path(path), ec))
		{
			out(5) << "Invalid path: \'" << path << "\'\n" << ec.message() << std::endl;
			continue;
		}
		for (auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(path)))
		{
#ifdef GUI
			if (entry.is_directory() || entry.path().extension() == ".zip")
			{
				entries.push_back(std::make_pair(true, entry));
				addpack(entry.path().filename().u8string());
				std::cout << entry.path().filename().u8string() << std::endl;
			}
#else
			if (entry.is_directory())
			{
				fsb(entry.path().u8string(), entry.path().filename().u8string(), false);
				vmt(entry.path().u8string(), entry.path().filename().u8string(), false);
				cim(entry.path().u8string(), entry.path().filename().u8string(), false);
			}
			else if (entry.path().extension() == ".zip")
			{
				fsb(entry.path().u8string(), entry.path().filename().u8string(), true);
				vmt(entry.path().u8string(), entry.path().filename().u8string(), true);
				cim(entry.path().u8string(), entry.path().filename().u8string(), true);
			}
#endif
		}
	}
#ifdef GUI
	ui->scroll->redraw();
	Fl::run();
#endif
	out(3) << "All Done!" << std::endl;
exit:;
#ifndef GUI
	if (pauseonexit)
	{
#ifdef _WIN32
		system("pause");
#else
		std::cout << "Press enter to continue . . .";
		getline(std::cin, str);
#endif
}
#endif
}
