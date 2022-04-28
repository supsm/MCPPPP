/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <filesystem>
#include <string>

namespace mcpppp
{
	// conversion status of resourcepack
	enum class checkresults { valid, noneconvertible, alrfound, reconverting };
	// results of check
	struct checkinfo
	{
		checkresults results;
		bool optifine;
		bool vmt_newlocation;
		bool iszip;
	};
}

namespace fsb
{
	// get conversion info for resourcepack
	// @param path  resourcepack to check
	// @param zip  whether resourcepack is zip
	// @return check results
	mcpppp::checkinfo check(const std::filesystem::path& path, bool zip) noexcept;

	// convert fsb part of resourcepack
	// @param path  resourcepack to convert
	// @param filename  filename of resourcepack
	// @param info  conversion info (check results)
	void convert(const std::filesystem::path& path, const std::u8string& filename, const mcpppp::checkinfo& info);
}

namespace vmt
{
	// get conversion info for resourcepack
	// @param path  resourcepack to check
	// @param zip  whether resourcepack is zip
	// @return check results
	mcpppp::checkinfo check(const std::filesystem::path& path, bool zip) noexcept;


	// convert vmt part of resourcepack
	// @param path  resourcepack to convert
	// @param filename  filename of resourcepack
	// @param info  conversion info (check results)
	void convert(const std::filesystem::path& path, const std::u8string& filename, const mcpppp::checkinfo& info);
}

namespace cim
{
	// get conversion info for resourcepack
	// @param path  resourcepack to check
	// @param zip  whether resourcepack is zip
	// @return check results
	mcpppp::checkinfo check(const std::filesystem::path& path, bool zip) noexcept;


	// convert cim part of resourcepack
	// @param path  resourcepack to convert
	// @param filename  filename of resourcepack
	// @param info  conversion info (check results)
	void convert(const std::filesystem::path& path, const std::u8string& filename, const mcpppp::checkinfo& info);
}
