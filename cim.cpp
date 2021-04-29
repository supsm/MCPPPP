/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

void cimother(std::string& folder, std::string& path, bool& zip, std::filesystem::directory_entry png)
{
	std::string folderpath = png.path().string();
	folderpath.erase(folderpath.begin(), folderpath.begin() + folderpath.rfind("/cit/") + 5);
	folderpath.erase(folderpath.end() - png.path().filename().string().size(), folderpath.end());
	for (int i = 0; i < folderpath.size(); i++)
	{
		if (folderpath[i] == '\\')
		{
			folderpath[i] = '/';
		}
	}
	std::filesystem::create_directories((zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/overrides/stuff/" + folderpath); // TODO: it must be better than "stuff"
	std::filesystem::copy(png.path().string(), (zip ? "mcpppp-temp/" + folder : path) + "/assets/minecraft/overrides/stuff/" + folderpath + png.path().filename().string());
}

void cimprop()
{

}

void cim(std::string path, std::string filename, bool zip)
{
	// source: assets/minecraft/*/cit (recursive)
	// destination: assets/minecraft/overrides

	std::string folder;
	bool optifine;
	Zippy::ZipArchive zipa;
	if (zip)
	{
		zipa.Open(path);
		if (zipa.HasEntry("assets/minecraft/overrides"))
		{
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		else if (zipa.HasEntry("assets/minecraft/optifine/cit/"))
		{
			optifine = true;
		}
		else if (zipa.HasEntry("assets/minecraft/mcpatcher/cit/"))
		{
			optifine = false;
		}
		else
		{
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		folder = filename;
		folder.erase(folder.end() - 4, folder.end());
		std::filesystem::create_directories("mcpppp-temp/" + folder);
		if (outputlevel <= 3)
		{
			std::cout << (dotimestamp ? timestamp() : "") << "CIM: Extracting " << filename << std::endl;
		}
		if (logfile.good() && loglevel <= 3)
		{
			logfile << timestamp() << "CIM: Extracting " << filename << std::endl;
		}
		zipa.ExtractEntry(std::string("assets/minecraft/") + (optifine ? "optifine" : "mcpatcher") + "/cit/", "mcpppp-temp/" + folder + '/');
	}
	else
	{
		if (std::filesystem::is_directory(path + "/assets/minecraft/overrides"))
		{
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "CIM: Chime folder found in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		else if (std::filesystem::is_directory(path + "/assets/minecraft/optifine/cit"))
		{
			optifine = true;
		}
		else if (std::filesystem::is_directory(path + "/assets/minecraft/mcpatcher/cit"))
		{
			optifine = false;
		}
		else
		{
			if (outputlevel <= 2)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			if (logfile.good() && loglevel <= 2)
			{
				logfile << timestamp() << "CIM: Nothing to convert in " << filename << ", skipping" << std::endl;
			}
			return;
		}
		if (outputlevel <= 3)
		{
			std::cout << (dotimestamp ? timestamp() : "") << "CIM: Converting Pack " << filename << std::endl;
		}
		if (logfile.good() && loglevel <= 3)
		{
			logfile << timestamp() << "CIM: Converting Pack " << filename << std::endl;
		}
	}
	for (auto& png : std::filesystem::recursive_directory_iterator(zip ? "mcpppp-temp/" + folder + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/cit" : path + "/assets/minecraft/" + (optifine ? "optifine" : "mcpatcher") + "/cit"))
	{
		if (png.path().extension() == ".png" || png.path().extension() == ".properties" || png.path().extension() == ".json")
		{
			if (outputlevel <= 1)
			{
				std::cout << (dotimestamp ? timestamp() : "") << "CIM: Converting " + png.path().filename().string() << std::endl;
			}
			if (logfile.good() && loglevel <= 1)
			{
				logfile << timestamp() << "CIM: Converting " + png.path().filename().string() << std::endl;
			}
		}
		if (png.path().extension() == ".json" || png.path().extension() == ".png")
		{
			cimother(folder, path, zip, png);
		}
	}
}