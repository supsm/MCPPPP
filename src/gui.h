/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#ifdef GUI
#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "convert.h"

namespace mcpppp
{
	inline bool dofsb = true; // perform fsb conversion (from checkbox)
	inline bool dovmt = true; // perform vmt conversion
	inline bool docim = true; // perform cim conversion

	inline bool running = false; // whether conversion is currently happening

	inline int numbuttons = 0; // number of resourcepack checkboxes in scroll box

	// paths that have been deleted through "Edit Paths" window
	inline std::set<std::filesystem::path> deletedpaths;

	// perform conversion
	void guirun();

	// add resourcepack to checklist
	// @param path  resourcepack to add
	// @param selected  whether to display the resourcepack as selected for conversion
	void addpack(const std::filesystem::path& path, const bool selected);

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

#ifdef _WIN32
	// file browser for windows, easier for the user to use than the one provided by Fl_Native_File_Chooser
	std::string winfilebrowser();
#endif
}
#endif
