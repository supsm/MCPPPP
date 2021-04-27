/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "utility.cpp"

#include "fsb.cpp"
#include "vmt.cpp"

int main(int argc, char* argv[])
{
	bool issetting = false, isvalue = false, extra;
	std::vector<std::string> paths;
	std::string str, option, value, temp;
	std::stringstream ss;
	std::error_code ec;
	std::ifstream config("mcpppp.properties");
	std::cout.sync_with_stdio(false);
	if (config.fail())
	{
		std::ofstream createconfig("mcpppp.properties");
		createconfig << "# MCPPPP will search folders for resource packs (such as your resourcepacks folder) and will edit the resource pack.\n# It won't touch anything but the necessary folders, and will skip the resourcepack if the folders already exist.\n# Enter a newline-seperated list of such folders" << std::endl;
		createconfig.close();
		std::cerr << (dotimestamp ? timestamp() : "") << "Config file not found, look for mcpppp.properties" << std::endl;
		goto exit;
	}
	while (config.good())
	{
		getline(config, str);
		if (str[0] == '/' && str[1] == '/')
		{
			ss.clear();
			ss.str(str);
			ss >> temp;
			if (temp == "//set")
			{
				ss >> option;
				getline(ss, value);
				value.erase(value.begin());
				setting(option, value);
			}
		}
		else if (str[0] != '#' && str != "")
		{
			paths.push_back(str);
		}
	}
	config.close();
	for (int i = 1; i < argc; i++)
	{
		if (issetting)
		{
			if (isvalue)
			{
				value = argv[i];
				extra = false;
				while (value[value.size() - 1] != ';' && i < argc - 1)
				{
					i++;
					value += ' ' + argv[i];
					extra = true;
				}
				if (extra)
				{
					i--;
				}
				value.erase(value.end() - 1);
				issetting = false;
				isvalue = false;
				setting(option, value);
			}
			else
			{
				option = argv[i];
				isvalue = true;
			}
		}
		else
		{
			if (argv[i] == "//set")
			{
				issetting = true;
			}
			else if (argv[i][0] == '#')
			{
				extra = false;
				temp = argv[i];
				while (temp[temp.size() - 1] != ';' && i < argc - 1)
				{
					i++;
					temp = argv[i];
					extra = true;
				}
				if (extra)
				{
					i--;
				}
			}
			else
			{
				extra = false;
				temp = argv[i];
				while (temp[temp.size() - 1] != ';' && i < argc - 1)
				{
					i++;
					temp += ' ' + argv[i];
					extra = true;
				}
				if (extra)
				{
					i--;
				}
				temp.erase(temp.end() - 1);
				paths.push_back(temp);
			}
		}
	}
	if (std::filesystem::is_directory("mcpppp-temp"))
	{
		if (autodeletetemp)
		{
			if (outputlevel <= 4)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "Folder named \"mcpppp-temp\" found. Removing..." << std::endl;
			}
			std::filesystem::remove_all("mcpppp-temp");
		}
		else
		{
			if (outputlevel <= 4)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "Folder named \"mcpppp-temp\" found. Please remove this folder." << std::endl;
			}
			goto exit;
		}
	}
	for (std::string path : paths)
	{
		if (!std::filesystem::is_directory(path, ec))
		{
			std::cerr << (dotimestamp ? timestamp() : "") << "Invalid path: \'" << path << "\'\n" << ec.message() << std::endl;
			continue;
		}
		for (auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory())
			{
				fsb(entry.path().string(), entry.path().filename().string(), false);
				vmt(entry.path().string(), entry.path().filename().string(), false);
			}
			else if (entry.path().extension() == ".zip")
			{
				fsb(entry.path().string(), entry.path().filename().string(), true);
				vmt(entry.path().string(), entry.path().filename().string(), true);
			}
		}
	}
	if (outputlevel <= 3)
	{
		std::cout << (dotimestamp ? timestamp() : "") << "All Done!" << std::endl;
	}
exit:
	if (pauseonexit)
	{
#ifdef _WIN32
		system("pause");
#else
		std::cout << (dotimestamp ? timestamp() : "") << "Press enter to continue . . .";
		getline(std::cin, str);
#endif
	}
}