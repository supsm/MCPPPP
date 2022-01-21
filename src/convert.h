/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <filesystem>
#include <string>

namespace mcpppp
{
	enum class checkresults { valid, noneconvertible, alrfound, reconverting };
	struct checkinfo
	{
		checkresults results;
		bool optifine, newlocation;
	};
}

namespace fsb
{
	mcpppp::checkinfo check(const std::filesystem::path& path, const bool& zip);
	void convert(const std::string& path, const std::string& filename, const mcpppp::checkinfo& info);
}

namespace vmt
{
	mcpppp::checkinfo check(const std::filesystem::path& path, const bool& zip);
	void convert(const std::string& path, const std::string& filename, const mcpppp::checkinfo& info);
}

namespace cim
{
	mcpppp::checkinfo check(const std::filesystem::path& path, const bool& zip);
	void convert(const std::string& path, const std::string& filename, const mcpppp::checkinfo& info);
}
