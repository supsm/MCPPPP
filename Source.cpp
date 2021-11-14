/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

 //#define GUI

constexpr auto VERSION = "0.5.6"; // MCPPPP version

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
#include <Windows.h> // SetProcessDpiAwarenessContext
#endif
#include "fl_impl.h"
#endif

using mcpppp::out;

int main(int argc, char* argv[])
try
{
	// we don't need this if running cli version, or if unaware_gdiscaled is not available
	// other dpi awareness settings make the window too small, it's probably better if blurry
#if defined _WIN32 && defined GUI && defined DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED); // fix blurriness
#endif
	std::string str, option, value, temp;
	std::error_code ec;
#ifdef GUI
	ui = std::make_unique<UI>();
	Fl::get_system_colors();
	mcpppp::ui->show();
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
			std::cerr << (mcpppp::dotimestamp ? mcpppp::timestamp() : "") << "Config file not found, look for mcpppp.properties" << std::endl;
			mcpppp::exit();
#endif
		}
		else
		{
			try
			{
				str.resize(std::filesystem::file_size("mcpppp-config.json"));
				configfile.read(&str.at(0), static_cast<std::streamsize>(std::filesystem::file_size("mcpppp-config.json")));
				mcpppp::config = nlohmann::ordered_json::parse(str, nullptr, true, true);
			}
			catch (const nlohmann::json::exception& e)
			{
				out(5) << e.what() << std::endl;
				mcpppp::exit();
			}
			mcpppp::readconfig();
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
			mcpppp::config = nlohmann::ordered_json::parse(str, nullptr, true, true);
		}
		catch (nlohmann::json::exception& e)
		{
			out(5) << e.what() << std::endl;
			mcpppp::exit();
		}
		mcpppp::readconfig();
	}
#endif

#ifdef GUI
	mcpppp::addpaths();
	mcpppp::updatepaths();
	mcpppp::dotimestamp = true;
	mcpppp::updatesettings();
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
	out(5) << "pauseOnExit     " << (mcpppp::pauseonexit ? "true" : "false") << std::endl;
	out(5) << "log             " << mcpppp::logfilename << std::endl;
	out(5) << "timestamp       " << (mcpppp::dotimestamp ? "true" : "false") << std::endl;
	out(5) << "autoDeleteTemp  " << (mcpppp::dotimestamp ? "true" : "false") << std::endl;
	out(5) << "outputLevel     " << mcpppp::outputlevel << std::endl;
	out(5) << "logLevel        " << mcpppp::loglevel << std::endl;
	out(5) << "autoReconvert   " << mcpppp::autoreconvert << std::endl << std::endl << std::endl;

	if (std::filesystem::is_directory("mcpppp-temp"))
	{
		if (mcpppp::autodeletetemp)
		{
			out(4) << "Folder named \"mcpppp-temp\" found. Removing..." << std::endl;
			std::filesystem::remove_all("mcpppp-temp");
		}
		else
		{
#ifdef GUI
			mcpppp::ui->tempfound->show();
#else
			out(5) << "Folder named \"mcpppp-temp\" found. Please remove this folder." << std::endl;
			mcpppp::exit();
#endif
		}
	}
	for (const std::string& path : mcpppp::paths)
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
				mcpppp::entries.emplace_back(std::make_pair(true, entry));
				mcpppp::addpack(entry.path().filename().u8string(), true);
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
				mcpppp::unzip(entry, zipa);
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
					mcpppp::rezip(folder, zipa);
				}
			}
#endif
		}
	}
#ifdef GUI
	mcpppp::ui->scroll->redraw();
	Fl::run();
#endif
	out(3) << "All Done!" << std::endl;
	mcpppp::exit();
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
