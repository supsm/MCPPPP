/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

//#define GUI

#define NOMINMAX

#define VERSION "0.4.5" // MCPPPP version

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
		std::ifstream configfile("mcpppp-config.json");
		if (configfile.fail() && argc < 2)
		{
			std::ofstream createconfig("mcpppp-config.json");
			createconfig << "// Please check out the Documentation for the config file before editing it yourself: https://github.com/supsm/MCPPPP/blob/master/CONFIG.md" << std::endl;
			createconfig.close();
#ifdef GUI
			openhelp(nullptr, nullptr);
#else
			std::cerr << (dotimestamp ? timestamp() : "") << "Config file not found, look for mcpppp.properties" << std::endl;
			goto exit;
#endif
		}
		else
		{
			try
			{
				str.resize(std::filesystem::file_size("mcpppp-config.json"));
				configfile.read((char*)(str.c_str()), std::filesystem::file_size("mcpppp-config.json"));
				config = nlohmann::ordered_json::parse(str, nullptr, true, true);
			}
			catch (nlohmann::json::exception& e)
			{
				out(5) << e.what() << std::endl;
				goto exit;
			}
			readconfig();
		}
	}
#ifndef GUI // gui doenst need command line options
	else
	{
		str.clear();
		str += argv[1];
		for (int i = 2; i < argc; i++)
		{
			str += ' ';
			str += argv[i];
		}
		try
		{
			config = nlohmann::ordered_json::parse(str, nullptr, true, true);
		}
		catch (nlohmann::json::exception& e)
		{
			out(5) << e.what() << std::endl;
			goto exit;
		}
		readconfig();
	}
#endif

#ifdef GUI
	addpaths();
	updatepaths();
	dotimestamp = true;
	updatesettings();
#endif

	out(5) << "MCPPPP " << VERSION
#ifdef GUI
		<< " (GUI)"
#else
		<< " (CLI)"
#endif
		<< std::endl;
	out(5) << "Os: " << 
#ifdef _WIN64
		"Win64"
#elif defined(_WIN32)
		"Win32"
#elif defined(__APPLE__) || defined(__MACH__)
		"Mac"
#elif defined(__linux__)
		"Linux"
#elif defined(__FreeBSD__)
		"FreeBSD"
#elif defind(__unix) || defined(__unix__)
		"Other Unix"
#else
		"Other"
#endif
		<< std::endl << std::endl;
	out(5) << "pauseOnExit     " << (pauseonexit ? "true" : "false") << std::endl;
	out(5) << "log             " << logfilename << std::endl;
	out(5) << "timestamp       " << (dotimestamp ? "true" : "false") << std::endl;
	out(5) << "autoDeleteTemp  " << (dotimestamp ? "true" : "false") << std::endl;
	out(5) << "outputLevel     " << outputlevel << std::endl;
	out(5) << "logLevel        " << loglevel << std::endl;
	out(5) << "autoReconvert   " << autoreconvert << std::endl << std::endl << std::endl;

	if (std::filesystem::is_directory("mcpppp-temp"))
	{
		if (autodeletetemp)
		{
			out(4) << "Folder named \"mcpppp-temp\" found. Removing..." << std::endl;
			std::filesystem::remove_all("mcpppp-temp");
		}
		else
		{
			out(5) << "Folder named \"mcpppp-temp\" found. Please remove this folder." << std::endl;
#ifdef GUI
			running = true; // prevent user from running
			Fl::run();
#else
			goto exit;
#endif
	}
	}
	for (const std::string& path : paths)
	{
		if (!std::filesystem::is_directory(std::filesystem::u8path(path), ec))
		{
			out(5) << "Invalid path: \'" << path << "\'\n" << ec.message() << std::endl;
			continue;
		}
		else
		{
			out(5) << "Path: " << path << std::endl;
		}
		for (auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(path)))
		{
#ifdef GUI
			if (entry.is_directory() || entry.path().extension() == ".zip")
			{
				entries.push_back(std::make_pair(true, entry));
				addpack(entry.path().filename().u8string(), true);
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
