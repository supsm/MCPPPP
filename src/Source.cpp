/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

 //#define GUI

 // VERSION and PACK_VER can be found in utility.h

#include "pch.h"

#include "constants.h"
#include "utility.h"

#ifdef GUI
#include <FL/fl_ask.H>
#include "gui.h"
#else
#include "convert.h"
#endif

using mcpppp::output;
using mcpppp::level_t;
using mcpppp::c8tomb;
using mcpppp::mbtoc8;
using mcpppp::checkpoint;

int main(int argc, const char* argv[])
try
{
	mcpppp::argc = argc;
	// we don't need this if running cli version, or if unaware_gdiscaled is not available
	// other dpi awareness settings make the window too small, it's probably better if blurry
#if defined _WIN32 && defined GUI && defined DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED); // fix blurriness
#endif


#ifdef __EMSCRIPTEN__ // skip all commandline and config file stuff for web interface
	// change default settings
	mcpppp::outputlevel = level_t::info;
	// we will treat console output (stdout) as log
	mcpppp::dolog = true;
	mcpppp::logfile.close();
	mcpppp::logfilename.clear();
	mcpppp::pauseonexit = false;
#else
	std::error_code ec;
	mcpppp::gethashes();
	if (argc < 2) // skip file settings if there are command line settings
	{
#ifdef GUI
		// also skip creating ui window

#ifdef _WIN32
		// hide console window on windows
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
		mcpppp::ui = std::make_unique<UI>();
		mcpppp::init_settings();
		Fl::get_system_colors();
		Fl::lock();
		fl_message_icon()->labeltype(FL_NO_LABEL);
		fl_message_icon()->box(FL_NO_BOX);
		mcpppp::ui->show();
		Fl::wait();

		checkpoint(); // finish gui init
#endif
		std::ifstream configfile("mcpppp-config.json");
		if (configfile.fail())
		{
			std::ofstream createconfig("mcpppp-config.json");
			createconfig << "// Please check out the Documentation for the config file before editing it yourself: https://github.com/supsm/MCPPPP/blob/master/CONFIG.md"
				<< std::endl << "{}" << std::endl;
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
				std::vector<char> contents{ std::istreambuf_iterator<char>(configfile), std::istreambuf_iterator<char>() };
				mcpppp::config = nlohmann::ordered_json::parse(contents, nullptr, true, true);
			}
			catch (const nlohmann::json::exception& e)
			{
				output<level_t::error>("Error while parsing config: {}", e.what());
				mcpppp::exit();
			}
			mcpppp::readconfig();
		}
		configfile.close();
		checkpoint(); // finish config stuff
	}
	else
	{
		mcpppp::parseargs(argc, argv);
		checkpoint(); // finish parsing arguments
	}

#ifdef GUI
	if (argc < 2)
	{
		mcpppp::addpaths();
		mcpppp::updatepaths();
		mcpppp::dotimestamp = true;
		mcpppp::updatesettings();
		checkpoint(); // finish gui config init
	}
#endif
#endif

	output<level_t::system_info>("MCPPPP {} {}", VERSION,
#ifdef GUI
		"(GUI)"
#elif defined(__EMSCRIPTEN__)
		"(WEB)"
#else
		"(CLI)"
#endif
		);
	output<level_t::system_info>("Os: {}\n",
#ifdef __EMSCRIPTEN__
		"Emscripten"
#elif defined(_WIN64)
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
		);
	// output settings and their values
	{
		// find longest name
		size_t longest_name_length = 0;
		for (const auto& s : mcpppp::settings)
		{
			longest_name_length = std::max(longest_name_length, s.second.formatted_name.size());
		}
		for (const auto& s : mcpppp::settings)
		{
			std::string setting_value;
			switch (s.second.type)
			{
			case mcpppp::type_t::boolean:
				setting_value = mcpppp::boolalpha(s.second.get<bool>());
				break;
			case mcpppp::type_t::integer:
				setting_value = std::to_string(static_cast<int>(s.second.get<level_t>()));
				break;
			case mcpppp::type_t::string:
				setting_value = s.second.get<std::string>();
				break;
			}
			output<level_t::system_info>("{}{}{}", s.second.formatted_name, std::string(longest_name_length - s.second.formatted_name.size() + 2, ' '), setting_value);
		}
		output<level_t::system_info>("\n");
	}
	checkpoint(); // finish outputting settings

	if (std::filesystem::is_directory("mcpppp-temp"))
	{
		if (mcpppp::autodeletetemp)
		{
			output<level_t::warning>("Folder named \"mcpppp-temp\" found. Removing...");
			std::filesystem::remove_all("mcpppp-temp");
		}
		else
		{
#ifdef GUI
			switch (fl_choice("mcpppp-temp folder was found. If you created it yourself, it may contain important files.\n\
				If you did not create it, it is probably the result of a failed conversion.\n\
				Do you want to delete it?", "Don't Delete", "Delete", nullptr))
			{
			case 0: // don't delete
				output<level_t::error>("Folder named \"mcpppp-temp\" found. Please remove this folder.");
				break;
			case 1: // delete
				std::filesystem::remove_all("mcpppp-temp");
				break;
			default:
				break;
			}
#else
			output<level_t::error>("Folder named \"mcpppp-temp\" found. Please remove this folder.");
			mcpppp::exit();
#endif
		}

		checkpoint(); // finish folder check
	}
#ifdef __EMSCRIPTEN__
	emscripten::val::global("window").call<void>("hide_loading");
	emscripten::val::global("window").call<void>("alert", std::string("Please upload a zipped resource pack, then press Start Conversion"));
#else
#ifndef GUI
	output<level_t::important>("Conversion Started");
#endif
	for (const std::filesystem::path& path : mcpppp::paths)
	{
		if (!std::filesystem::is_directory(path, ec))
		{
			output<level_t::error>("Invalid path: \'{}\'\n{}", c8tomb(path.generic_u8string()), ec.message());
			continue;
		}
		output<level_t::info>("Path: {}", c8tomb(path.generic_u8string()));
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory() || entry.path().extension() == ".zip")
			{
#ifdef GUI
				mcpppp::addpack(entry, true);
#else
				mcpppp::entries.emplace_back(entry);
#endif
				checkpoint(); // finish resource pack addition
			}
		}
	}
#endif
#ifdef GUI
	if (argc < 2)
	{
		mcpppp::ui->scroll->redraw();
		Fl::run();
	}
	else
	{
		for (const auto& entry : mcpppp::entries)
		{
			mcpppp::convert(entry.path_entry);
			checkpoint(); // finish conversion
		}
	}
#elif !defined(__EMSCRIPTEN__)
	for (const auto& entry : mcpppp::entries)
	{
		mcpppp::convert(entry.path_entry);
		checkpoint(); // finish conversion
	}
	output<level_t::important>("Conversion Finished");
	mcpppp::exit();
#endif
}
MCPPPP_CATCH_ALL()
