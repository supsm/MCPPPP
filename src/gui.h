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
	inline std::set<std::filesystem::path> deletedpaths;

	void guirun();

	void addpack(const std::filesystem::path& path, bool selected);

	void updatepaths();

	void updatesettings();

	void updatepathconfig();

	void addpaths();

	void addpath(const std::filesystem::path& path);

#ifdef _WIN32
	std::string winfilebrowser();
#endif
}
#endif
