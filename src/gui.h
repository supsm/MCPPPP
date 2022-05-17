/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "pch.h"

#ifdef GUI

#include "convert.h"
#include "utility.h"

#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Input.H>

namespace mcpppp
{
	// colors to use for label text based on conversion status
	const inline std::array<Fl_Color, 4> result_colors = { FL_DARK_GREEN, FL_DARK3, FL_RED, 93 };

	inline bool dofsb = true; // perform fsb conversion (from checkbox)
	inline bool dovmt = true; // perform vmt conversion
	inline bool docim = true; // perform cim conversion

	inline bool running = false; // whether conversion is currently happening

	inline int numbuttons = 0; // number of resourcepack checkboxes in scroll box

	// FLTK widgets for settings
	inline std::unordered_map<std::string, std::variant
		<
		std::pair<Fl_Button*, Fl_Button*>, // pair of true and false buttons
		Fl_Counter*, // counter (text input + increment/decrement)
		Fl_Input* // text input
		>> settings_widgets;

	inline Fl_Box* savewarning;

	// paths that have been deleted through "Edit Paths" window
	inline std::set<std::filesystem::path> deletedpaths;

	// perform conversion
	void guirun();

	// combine all/multiple checkresults into a single checkresult
	// @param all_checkresults  map of checkresults to combine (maybe from getconvstatus)
	checkresults combine_checkresults(const std::unordered_map<conversions, checkresults>& all_checkresults);

	// get string to use as tooltip for resourcepacks in checklists
	// @param result  combined checkresults (use combine_checkresults)
	// @param forcereconvert  whether to force reconvert pack, changes text if `result` is reconverting
	// @param path  path of resource pack
	// @return tooptip to use
	std::string get_pack_tooltip_name(checkresults result, bool forcereconvert, const std::filesystem::path& path);

	// add resourcepack to checklist and entries
	// @param path  directory entry of resourcepack to add
	// @param selected  whether to display the resourcepack as selected for conversion
	void addpack(const std::filesystem::directory_entry& path, const bool selected);

	// update paths from "Edit Paths" to path_input
	void updatepaths();

	// update settings in "Settings"
	void updatesettings();

	// update config file to include paths
	void updatepathconfig();

	// add paths to "Edit Paths" from paths
	void addpaths();

	// add a single path to "Edit Paths" AND paths
	// @param path  path to add
	void addpath(const std::filesystem::path& path);

	// initialize settings_widgets
	void init_settings();

#ifdef _WIN32
	// file browser for windows, easier for the user to use than the one provided by Fl_Native_File_Chooser
	std::string winfilebrowser();
#endif
}
#endif
