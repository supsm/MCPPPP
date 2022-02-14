/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "constants.h"
#include "reselect.h"
#include "convert.h"
#include "utility.h"

using mcpppp::out;
using mcpppp::c8tomb;
using mcpppp::mbtoc8;

// temporary thing so i can compile
#define VMT ""

namespace vmt
{
	static std::unordered_set<std::string> png_names;
	static std::unordered_set<std::u8string> png_filenames;

	class s_mob_t
	{
	public:
		std::string type, reselect_func;
		reselect data;
	};
	static std::map<std::string, std::vector<s_mob_t>> s_mobs; // special mobs, but that name is already used

	// separate name and number (e.g. zombie12 -> zombie, 12)
	// @param str  filename to separate
	// @return pair of string (name) and int
	static std::pair<std::string, int> separate(const std::string& str)
	{
		std::string name = str, num;
		for (size_t i = str.size(); i > 0; i--)
		{
			if (std::isdigit(str[i - 1]) != 0)
			{
				num.push_back(str[i - 1]);
			}
			else
			{
				name.erase(name.begin() + i, name.end());
				break;
			}
		}
		if (num.empty())
		{
			return { name, -1 };
		}
		else
		{
			std::reverse(num.begin(), num.end());
			return { name, std::stoi(num) };
		}
	}

	// moves vmt pngs to new location
	// @param path  path of resourcepack being converted
	// @param optifine  whether to use optifine locations (no = mcpatcher)
	// @param newlocation  whether to use new optifine location
	// @param entry  actual png file to convert
	static void png(const std::u8string& path, const bool& optifine, const bool& newlocation, const std::filesystem::directory_entry& entry)
	{
		const auto p = separate(c8tomb(entry.path().filename().stem().u8string()));
		const std::string curname = p.first;
		// current name is new, look for all textures
		if (p.second != -1 && !png_names.contains(curname))
		{
			std::filesystem::path folderpath;
			std::vector<reselect> paths = { reselect("default", false) };

			{
				// get directory that files are in
				const std::filesystem::path location = (optifine ? (newlocation ? u8"assets/minecraft/optifine/random/entity" : u8"assets/minecraft/optifine/mob") : u8"assets/minecraft/mcpatcher/mob");
				// I'd prefer to use std::filesystem::relative here, but given 2 equal paths it returns "." instead of ""
				std::u8string tempfolderpath = std::filesystem::canonical(entry.path().parent_path()).generic_u8string();
				tempfolderpath.erase(tempfolderpath.begin(),
					tempfolderpath.begin() + std::filesystem::canonical(path / location).generic_u8string().size() + 1);
				folderpath = tempfolderpath;
			}

			// search for name1.png, name2.png, etc
			for (int curnum = 2; curnum < std::numeric_limits<int>::max(); curnum++)
			{
				const std::u8string curfilename = mbtoc8(curname + std::to_string(curnum) + ".png");
				const std::filesystem::path texture = entry.path().parent_path() / curfilename;
				// doesn't exist, textures have ended
				if (!std::filesystem::exists(texture))
				{
					break;
				}

				mcpppp::copy(texture, std::filesystem::path(path + u8"/assets/mcpppp/vmt/") / folderpath / curfilename);

				paths.push_back(static_cast<reselect>(c8tomb((u8"mcpppp:vmt/" / folderpath / curfilename).generic_u8string())));

				png_filenames.insert((folderpath / curfilename).generic_u8string());
			}
			png_names.insert(curname);

			// randomly select between each texture, if there is more than just the default path
			if (paths.size() > 1)
			{
				reselect res;
				res.add_random(curname, paths);
				const std::string str = res.get_string();
				std::ofstream fout(std::filesystem::path(path) / "assets/vmt" / (curname + ".reselect"));
				fout.write(str.c_str(), str.size());
				fout.close();
			}
		}
		else
		{
			std::filesystem::path folderpath;
			{
				const std::filesystem::path location = (optifine ? (newlocation ? u8"assets/minecraft/optifine/random/entity" : u8"assets/minecraft/optifine/mob") : u8"assets/minecraft/mcpatcher/mob");
				std::u8string tempfolderpath = std::filesystem::canonical(entry.path().parent_path()).generic_u8string();
				tempfolderpath.erase(tempfolderpath.begin(),
					tempfolderpath.begin() + std::filesystem::canonical(path / location).generic_u8string().size() + 1);
				folderpath = tempfolderpath;
			}

			if (!png_filenames.contains((folderpath / entry.path().filename()).generic_u8string()))
			{
				mcpppp::copy(entry.path(), std::filesystem::path(path + u8"/assets/mcpppp/vmt/") / folderpath / entry.path().filename().u8string());
				png_filenames.insert((folderpath / entry.path().filename()).generic_u8string());
			}
		}
	}

	enum class match_type { normal, regex, iregex };

	static void read_prop(const bool& newlocation, const std::filesystem::directory_entry& entry,
		std::string& name,
		std::u8string& folderpath,
		std::vector<std::vector<std::string>>& textures,
		std::vector<std::vector<int>>& weights,
		std::vector<std::vector<std::string>>& biomes,
		std::vector<std::vector<std::pair<std::string, std::string>>>& heights,
		std::vector<double>& minheight,
		std::vector<double>& maxheight,
		std::vector<std::pair<std::string, match_type>>& names,
		std::vector<int>& baby,
		std::vector<std::vector<std::pair<std::string, std::string>>>& times,
		std::vector<std::array<bool, 4>>& weather)
	{
		long long curnum;
		// TODO: there's probably a better way to do this (folderpath)
		folderpath = entry.path().generic_u8string();
		folderpath.erase(folderpath.begin(), folderpath.begin() + static_cast<std::string::difference_type>(folderpath.rfind(newlocation ? u8"/random/entity/" : u8"/mob/") + (newlocation ? 15 : 5)));
		folderpath.erase(folderpath.end() - static_cast<std::string::difference_type>(entry.path().filename().u8string().size()), folderpath.end());
		name = c8tomb(entry.path().stem().u8string());
		std::string temp, option, value, time1, height1, tempnum;
		std::stringstream ss;
		std::ifstream fin(entry.path());
		while (fin)
		{
			std::getline(fin, temp);
			option.clear();
			value.clear();
			bool isvalue = false;
			if (temp.empty() || temp.front() == '#')
			{
				continue;
			}
			for (const char& c : temp)
			{
				if (c == '=')
				{
					isvalue = true;
				}
				else if (!isvalue)
				{
					option += c;
				}
				else // isvalue
				{
					value += c;
				}
			}
			tempnum.clear();
			for (size_t i = option.size(); i >= 1; i--)
			{
				if (option.at(i - 1) >= '0' && option.at(i - 1) <= '9')
				{
					tempnum.insert(tempnum.begin(), option.at(i - 1));
				}
				else
				{
					break;
				}
			}
			if (tempnum.empty())
			{
				continue;
			}
			curnum = stoi(tempnum);
			if (static_cast<size_t>(curnum) > textures.size())
			{
				textures.resize(static_cast<size_t>(curnum));
				weights.resize(static_cast<size_t>(curnum));
				biomes.resize(static_cast<size_t>(curnum));
				times.resize(static_cast<size_t>(curnum));
				baby.resize(static_cast<size_t>(curnum), -1);
				heights.resize(static_cast<size_t>(curnum));
				names.resize(static_cast<size_t>(curnum), std::make_pair("", match_type::normal));
				weather.resize(static_cast<size_t>(curnum), { false, false, false, false });
				minheight.resize(static_cast<size_t>(curnum), std::numeric_limits<double>::min());
				maxheight.resize(static_cast<size_t>(curnum), std::numeric_limits<double>::min());
			}
			if (option.starts_with("textures.") || option.starts_with("skins."))
			{
				// clear if already has elements (should not happen!)
				if (!textures.at(static_cast<size_t>(curnum - 1)).empty())
				{
					if (option.starts_with('t')) // starts with textures
					{
						out(2) << "(warn) VMT: Duplicate predicate textures." << curnum << " in " << c8tomb(entry.path().generic_u8string()) << std::endl;
					}
					else
					{
						out(2) << "(warn) VMT: Duplicate predicate skins." << curnum << " in " << c8tomb(entry.path().generic_u8string()) << std::endl;
					}
					textures.at(static_cast<size_t>(curnum - 1)).clear();
				}
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (!temp.empty())
					{
						if (temp == "1")
						{
							textures.at(static_cast<size_t>(curnum - 1)).push_back(std::string());
						}
						else
						{
							textures.at(static_cast<size_t>(curnum - 1)).push_back(c8tomb(u8"mcpppp:vmt/" + folderpath + mbtoc8(name) + mbtoc8(temp) + u8".png"));
						}
					}
				}
			}
			else if (option.starts_with("weights."))
			{
				// clear if already has elements (should not happen!)
				if (!weights.at(static_cast<size_t>(curnum - 1)).empty())
				{
					out(2) << "(warn) VMT: Duplicate predicate weights." << curnum << " in " << c8tomb(entry.path().generic_u8string()) << std::endl;
					weights.at(static_cast<size_t>(curnum - 1)).clear();
				}
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (!temp.empty())
					{
						weights.at(static_cast<size_t>(curnum - 1)).push_back(stoi(temp));
					}
				}
			}
			else if (option.starts_with("biomes."))
			{
				// clear if already has elements (should not happen!)
				if (!biomes.at(static_cast<size_t>(curnum - 1)).empty())
				{
					out(2) << "(warn) VMT: Duplicate predicate biomes." << curnum << " in " << c8tomb(entry.path().generic_u8string()) << std::endl;
					biomes.at(static_cast<size_t>(curnum - 1)).clear();
				}
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (!temp.empty())
					{
						// std::string::contains in C++23
						if (temp.find(':') == std::string::npos) // does not contain a namespace
						{
							// binary search for biome name
							const auto it = std::lower_bound(biomelist.begin(), biomelist.end(), temp, [](const std::string& a, const std::string& b) -> bool
								{
									return mcpppp::lowercase(mcpppp::ununderscore(a)) < mcpppp::lowercase(mcpppp::ununderscore(b));
								});
							if (it == biomelist.end() || (mcpppp::ununderscore(*it) != mcpppp::lowercase(mcpppp::ununderscore(temp))))
							{
								out(2) << "(warn) Invalid biome name: " << temp << std::endl;
							}
							else
							{
								biomes.at(static_cast<size_t>(curnum - 1)).push_back("minecraft:" + *it);
							}
						}
						else // contains a namespace
						{
							biomes.at(static_cast<size_t>(curnum - 1)).push_back(temp);
						}
					}
				}
			}
			else if (option.starts_with("heights."))
			{
				// clear if already has elements (should not happen!)
				if (!heights.at(static_cast<size_t>(curnum - 1)).empty())
				{
					out(2) << "(warn) VMT: Duplicate predicate heights." << curnum << " in " << c8tomb(entry.path().generic_u8string()) << std::endl;
					heights.at(static_cast<size_t>(curnum - 1)).clear();
				}
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (!temp.empty())
					{
						height1.clear();
						for (size_t i = 0; i < temp.size(); i++)
						{
							if (temp.at(i) == '-')
							{
								temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i));
								break;
							}
							height1 += temp.at(i);
						}
						heights.at(static_cast<size_t>(curnum - 1)).push_back(std::make_pair(height1, temp));
					}
				}
			}
			else if (option.starts_with("minHeight"))
			{
				try
				{
					minheight.at(static_cast<size_t>(curnum - 1)) = std::stod(value);
				}
				catch (const std::invalid_argument& e)
				{
					out(5) << "VMT Error: " << e.what() << std::endl << "stod argument: " << value;
				}
			}
			else if (option.starts_with("maxHeight"))
			{
				try
				{
					maxheight.at(static_cast<size_t>(curnum - 1)) = std::stod(value);
				}
				catch (const std::invalid_argument& e)
				{
					out(5) << "VMT Error: " << e.what() << std::endl << "stod argument: " << value;
				}
			}
			else if (option.starts_with("name."))
			{
				temp = value;
				match_type type = match_type::normal;
				if (temp.starts_with("regex:") || temp.starts_with("iregex:") || temp.starts_with("pattern:") || temp.starts_with("ipattern:"))
				{
					// if first character is i, then it is iregex (case insensitive)
					type = (temp.front() == 'i' ? match_type::iregex : match_type::regex);
					for (size_t i = 0; i < temp.size(); i++)
					{
						if (temp.at(i) == ':')
						{
							temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i + 1));
							break;
						}
					}
					if (temp.starts_with("pattern:") || temp.starts_with("ipattern:"))
					{
						temp = mcpppp::oftoregex(temp);
					}
				}
				names.at(static_cast<size_t>(curnum - 1)) = std::make_pair(temp, type);
			}
			else if (option.starts_with("professions."))
			{
				// not sure this is possible
			}
			else if (option.starts_with("collarColors."))
			{
				// not sure this is possible
			}
			else if (option.starts_with("baby."))
			{
				if (value == "true")
				{
					baby.at(static_cast<size_t>(curnum - 1)) = 1;
				}
				else if (value == "false")
				{
					baby.at(static_cast<size_t>(curnum - 1)) = 0;
				}
			}
			else if (option.starts_with("health."))
			{
				// TODO
			}
			else if (option.starts_with("moonPhase."))
			{
				// not sure this is possible
			}
			else if (option.starts_with("dayTime."))
			{
				// clear if already has elements (should not happen!)
				if (!times.at(static_cast<size_t>(curnum - 1)).empty())
				{
					out(2) << "(warn) VMT: Duplicate predicate dayTime." << curnum << " in " << c8tomb(entry.path().generic_u8string()) << std::endl;
					times.at(static_cast<size_t>(curnum - 1)).clear();
				}
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (!temp.empty())
					{
						time1.clear();
						for (size_t i = 0; i < temp.size(); i++)
						{
							if (temp.at(i) == '-')
							{
								temp.erase(temp.begin(), temp.begin() + static_cast<std::string::difference_type>(i));
								break;
							}
							time1 += temp.at(i);
						}
						times.at(static_cast<size_t>(curnum - 1)).push_back(std::make_pair(time1, temp));
					}
				}
			}
			else if (option.starts_with("weather."))
			{
				ss.clear();
				ss.str(value);
				while (ss)
				{
					temp = "";
					ss >> temp;
					if (temp == "clear")
					{
						weather.at(static_cast<size_t>(curnum - 1)).at(1) = true;
						weather.at(static_cast<size_t>(curnum - 1)).at(0) = true;
					}
					if (temp == "rain")
					{
						weather.at(static_cast<size_t>(curnum - 1)).at(2) = true;
						weather.at(static_cast<size_t>(curnum - 1)).at(0) = true;
					}
					if (temp == "thunder")
					{
						weather.at(static_cast<size_t>(curnum - 1)).at(3) = true;
						weather.at(static_cast<size_t>(curnum - 1)).at(0) = true;
					}
				}
			}
		}
	}

	// converts optifine properties to vmt/reselect
	static void prop(const std::u8string& path, const bool& newlocation, const std::filesystem::directory_entry& entry)
	{
		std::string name;
		std::u8string folderpath;
		std::vector<std::vector<std::string>> textures;
		std::vector<std::vector<int>> weights;
		std::vector<std::vector<std::string>> biomes;
		std::vector<std::vector<std::pair<std::string, std::string>>> heights;
		std::vector<double> minheight, maxheight;
		std::vector<std::pair<std::string, match_type>> names;
		std::vector<int> baby;
		std::vector<std::vector<std::pair<std::string, std::string>>> times;
		std::vector<std::array<bool, 4>> weather;
		read_prop(newlocation, entry, name, folderpath, textures, weights, biomes, heights, minheight, maxheight, names, baby, times, weather);
		if (!folderpath.empty())
		{
			folderpath.pop_back(); // remove trailing /
		}

		// check that mob is valid
		int sm_ind = -1;
		std::string raw_type;
		if (!std::binary_search(mobs.begin(), mobs.end(), name))
		{
			bool found = false;
			for (int i = 0; i < special_mobs.size(); i++)
			{
				if (special_mobs[i] == std::make_pair(name, c8tomb(folderpath)))
				{
					// if matches, set index and change name
					sm_ind = i;
					const auto p = special_mobs[i].split(name);
					name = p.first;
					raw_type = p.second;
					break;
				}
			}
			if (sm_ind == -1)
			{
				out(2) << "(warn) VMT: Invalid/unsupported mob: " << name << std::endl;
				return;
			}
		}

		reselect res, defaultstatement = reselect("default", false);
		std::vector<reselect> conditions;
		std::vector<reselect> statements;
		for (int i = 0; i < textures.size(); i++)
		{
			if (textures.at(i).empty())
			{
				out(2) << "(warn) VMT: textures." << i + 1 << " is empty in " << c8tomb(entry.path().generic_u8string()) << std::endl;
				continue;
			}
			reselect temp_res;
			std::vector<reselect> temp_conditions;
			if (textures.at(i).size() == 1)
			{
				// don't need to select randomly if there is only 1
				if (textures.at(i).front().empty())
				{
					// empty texture = default
					temp_res.push_back("default");
				}
				else
				{
					temp_res.push_back('\"' + textures.at(i).front() + '\"');
				}
			}
			else
			{
				temp_res.add_random(name, textures.at(i), weights.at(i));
			}

			// order is important! faster things should be evaluated first, to take advantage of short-circuit evaluation
			// as of 2/9/22, the cost is "regex >>> string comparison > anything else that operates on strings > everything else"
			if (!heights.at(i).empty())
			{
				std::vector<std::string> tempv;
				std::transform(heights.at(i).begin(), heights.at(i).end(), std::back_inserter(tempv), [&name](std::pair<std::string, std::string> p) -> std::string
					{
						// make sure double is valid
						try
						{
							double d = std::stod(p.first);
							double d2 = std::stod(p.second);
						}
						catch (const std::invalid_argument& e)
						{
							out(5) << "VMT Error: " << e.what() << std::endl << "stod argument: " << p.first << ", " << p.second << std::endl;
							return std::string();
						}

						// make it a decimal
						// std::string::contains in C++23
						if (p.first.find('.') == std::string::npos)
						{
							p.first += ".0";
						}
						if (p.second.find('.') == std::string::npos)
						{
							p.second += ".0";
						}
						return '(' + name + ".y >= " + p.first + " and " +
							name + ".y <= " + p.second + ')';
					});
				temp_conditions.push_back(reselect::construct_or(tempv));
			}
			else if (minheight.at(i) != std::numeric_limits<double>::min() || maxheight.at(i) != std::numeric_limits<double>::min()) // heights.n overrides minHeight, maxHeight
			{
				std::stringstream ss;
				if (minheight.at(i) != std::numeric_limits<double>::min())
				{
					ss << std::fixed << std::setprecision(2) <<
						name << ".y >= " << minheight.at(i);
				}
				if (maxheight.at(i) != std::numeric_limits<double>::min())
				{
					if (!ss.str().empty())
					{
						ss << " and ";
					}
					ss << std::fixed << std::setprecision(2) <<
						name << ".y <= " << maxheight.at(i);
				}
				temp_conditions.push_back(reselect(ss.str(), false));
			}
			if (baby.at(i) != -1)
			{
				if (baby.at(i))
				{
					temp_conditions.push_back(reselect(name + ".is_baby", false));
				}
				else
				{
					temp_conditions.push_back(reselect("not " + name + ".is_baby", false));
				}
			}
			if (!biomes.at(i).empty())
			{
				std::vector<std::string> tempv;
				std::transform(biomes.at(i).begin(), biomes.at(i).end(), std::back_inserter(tempv), [&name](const std::string& s) -> std::string
					{
						return name + ".current_biome == \"" + s + '\"';
					});
				temp_conditions.push_back(reselect::construct_or(tempv));
			}
			if (!names.at(i).first.empty())
			{
				std::string condition = name + ".has_name and ";
				switch (names.at(i).second)
				{
				case match_type::normal:
					condition += name + ".name == \"" + names.at(i).first + "\"";
					break;
				case match_type::regex:
					condition += name + ".name.matches(\"" + names.at(i).first + "\")";
					break;
				case match_type::iregex:
					condition += name + ".name.lowercase.matches(\"" + mcpppp::lowercase(names.at(i).first) + "\")";
					break;
				}
				temp_conditions.push_back(reselect(condition, false));
			}

			if (temp_conditions.empty())
			{
				defaultstatement = temp_res;
				break;
			}
			else
			{
				conditions.push_back(reselect::construct_and(temp_conditions));
			}
			statements.push_back(temp_res);
		}

		res.add_if(conditions, statements, defaultstatement);

		if (res.empty())
		{
			out(2) << "(warn) VMT: No predicates found in " << c8tomb(entry.path().generic_u8string()) << std::endl;
			return;
		}

		if (sm_ind == -1) // if normal supported mob
		{
			std::filesystem::create_directories(path + u8"/assets/vmt/");
			std::ofstream fout(std::filesystem::path(path + u8"/assets/vmt/" + mbtoc8(name) + u8".reselect"));
			fout << res.get_string() << std::endl;
			fout.close();
		}
		else
		{
			const special_mob s = special_mobs.at(sm_ind);
			const std::string type = s.get_typename(raw_type);
			s_mobs[name].push_back({ type, s.reselect_func, res });
		}
	}

	// check if should be converted
	mcpppp::checkinfo check(const std::filesystem::path& path, const bool& zip)
	{
		using mcpppp::checkresults;
		bool reconverting = false;
		if (mcpppp::findfolder(path.u8string(), u8"assets/vmt/", zip))
		{
			if (mcpppp::autoreconvert)
			{
				reconverting = true;
			}
			else
			{
				return { checkresults::alrfound, false, false };
			}
		}
		if (mcpppp::findfolder(path.u8string(), u8"assets/minecraft/optifine/random/entity/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, true, true };
			}
			else
			{
				return { checkresults::valid, true, true };
			}
		}
		else if (mcpppp::findfolder(path.u8string(), u8"assets/minecraft/optifine/mob/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, true, false };
			}
			else
			{
				return { checkresults::valid, true, false };
			}
		}
		else if (mcpppp::findfolder(path.u8string(), u8"assets/minecraft/mcpatcher/mob/", zip))
		{
			if (reconverting)
			{
				return { checkresults::reconverting, false, false };
			}
			else
			{
				return { checkresults::valid, false, false };
			}
		}
		else
		{
			return { checkresults::noneconvertible, false, false };
		}
	}

	// main vmt function
	void convert(const std::u8string& path, const std::u8string& filename, const mcpppp::checkinfo& info)
	{
		// source: assets/minecraft/*/mob/		< this can be of or mcpatcher, but the one below is of only
		// source: assets/minecraft/optifine/random/entity/
		// destination: assets/mcpppp/vmt/

		out(3) << "VMT: Converting Pack " << c8tomb(filename) << std::endl;

		// convert all images first, so the reselect file can be overridden
		for (const auto& entry : std::filesystem::recursive_directory_iterator(
			std::filesystem::path(path + u8"/assets/minecraft/" +
				(info.optifine ? u8"optifine" + std::u8string(info.newlocation ? u8"/random/entity/" : u8"/mob/") : u8"mcpatcher/mob/"))))
		{
			if (entry.path().extension() == ".png")
			{
				out(1) << "VMT: Converting " + c8tomb(entry.path().filename().u8string()) << std::endl;
			}
			if (entry.path().filename().extension() == ".png")
			{
				png(path, info.optifine, info.newlocation, entry);
			}
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(
			std::filesystem::path(path + u8"/assets/minecraft/" +
				(info.optifine ? u8"optifine" + std::u8string(info.newlocation ? u8"/random/entity/" : u8"/mob/") : u8"mcpatcher/mob/"))))
		{
			if (entry.path().extension() == ".properties")
			{
				out(1) << "VMT: Converting " + c8tomb(entry.path().filename().u8string()) << std::endl;
			}
			if (entry.path().filename().extension() == ".properties")
			{
				prop(path, info.newlocation, entry);
			}
		}

		// output special mob reselect files
		for (const auto& p : s_mobs)
		{
			reselect res;
			std::vector<std::string> conditions;
			std::vector<reselect> statements;
			for (const auto& m : p.second)
			{
				conditions.push_back(p.first + '.' + m.reselect_func + " == \"" + m.type + '\"');
				statements.push_back(m.data);
			}

			res.add_if(conditions, statements);

			std::filesystem::create_directories(path + u8"/assets/vmt/");
			std::ofstream fout(std::filesystem::path(path + u8"/assets/vmt/" + mbtoc8(p.first) + u8".reselect"));
			fout << res.get_string() << std::endl;
			fout.close();
		}
	}
};
