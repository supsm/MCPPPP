/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

 //#define GUI

constexpr auto VERSION = "0.5.5"; // MCPPPP version

#ifdef _WIN32
#define NOMINMAX
#endif

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#ifndef GUI
#include "fsb.h"
#include "vmt.h"
#include "cim.h"
#else
#if defined(_WIN32)
#include <Windows.h> // SetProcessDPIAware
#endif
#include "gui.h"
#endif

int main(int argc, char* argv[])
try
{
#if defined _WIN32 && defined GUI
	SetProcessDPIAware(); // fix blurriness
#endif
	std::string str, option, value, temp;
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
			createconfig << "// Please check out the Documentation for the config file before editing it yourself: https://github.com/supsm/MCPPPP/blob/master/CONFIG.md" << std::endl << "{}" << std::endl;
			createconfig.close();
#ifdef GUI
			openhelp(nullptr, nullptr);
#else
			std::cerr << (dotimestamp ? timestamp() : "") << "Config file not found, look for mcpppp.properties" << std::endl;
			exit();
#endif
		}
		else
		{
			try
			{
				str.resize(std::filesystem::file_size("mcpppp-config.json"));
				configfile.read(&str.at(0), static_cast<std::streamsize>(std::filesystem::file_size("mcpppp-config.json")));
				config = nlohmann::ordered_json::parse(str, nullptr, true, true);
			}
			catch (const nlohmann::json::exception& e)
			{
				out(5) << e.what() << std::endl;
				exit();
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
			exit();
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
#elif defined(__unix) || defined(__unix__)
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
#ifdef GUI
			ui->tempfound->show();
#else
			out(5) << "Folder named \"mcpppp-temp\" found. Please remove this folder." << std::endl;
			exit();
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
		out(2) << "Path: " << path << std::endl;
		for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(path)))
		{
#ifdef GUI
			if (entry.is_directory() || entry.path().extension() == ".zip")
			{
				entries.emplace_back(std::make_pair(true, entry));
				addpack(entry.path().filename().u8string(), true);
			}
#else
			if (entry.is_directory())
			{
				fsb(entry.path().u8string(), entry.path().filename().u8string());
				vmt(entry.path().u8string(), entry.path().filename().u8string());
				cim(entry.path().u8string(), entry.path().filename().u8string());
			}
			else if (entry.path().extension() == ".zip")
			{
				bool success = false;
				Zippy::ZipArchive zipa;
				unzip(entry, zipa);
				std::string folder = entry.path().stem().u8string();
				if (fsb("mcpppp-temp/" + folder, folder).success)
				{
					success = true;
				}
				if (vmt("mcpppp-temp/" + folder, folder).success)
				{
					success = true;
				}
				if (cim("mcpppp-temp/" + folder, folder).success)
				{
					success = true;
				}
				if (success)
				{
					rezip(folder, zipa);
				}
			}
#endif
		}
	}
#ifdef GUI
	ui->scroll->redraw();
	Fl::run();
#endif
	out(3) << "All Done!" << std::endl;
	exit();
}
catch (const nlohmann::json::exception& e)
{
	out(5) << "FATAL JSON ERROR:" << std::endl << e.what() << std::endl;
}
catch (const Zippy::ZipLogicError& e)
{
	out(5) << "FATAL ZIP LOGIC ERROR" << std::endl << e.what() << std::endl;
}
catch (const Zippy::ZipRuntimeError& e)
{
	out(5) << "FATAL ZIP RUNTIME ERROR" << std::endl << e.what() << std::endl;
}
catch (const std::filesystem::filesystem_error& e)
{
	out(5) << "FATAL FILESYSTEM ERROR:" << std::endl << e.what() << std::endl;
}
catch (const std::exception& e)
{
	out(5) << "FATAL ERROR:" << std::endl << e.what() << std::endl;
}
catch (...)
{
	out(5) << "UNKNOWN FATAL ERROR" << std::endl;
}
