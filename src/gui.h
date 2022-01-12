/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#ifdef GUI
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace mcpppp
{
	inline bool dofsb = true, dovmt = true, docim = true, running = false;
	inline int numbuttons = 0;
	inline std::vector<std::pair<bool, std::filesystem::directory_entry>> entries = {};
	inline std::set<std::string> deletedpaths;

	std::string getdefaultpath();

	void guirun();

	double getscale();

	void addpack(const std::string& name, bool selected);

	void updatepaths();

	void updatesettings();

	void updatepathconfig();

	void addpaths();

	void addpath(const std::string& name);

#ifdef _WIN32
	std::string winfilebrowser();
#endif
}
#endif
