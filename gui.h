/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef GUI
#include <future>

#include "fsb.h"
#include "vmt.h"
#include "cim.h"

#ifdef _WIN32
#define WIN32 // fltk wants this for some reason
#endif

extern void resourcepack(Fl_Check_Button*, void*);
extern void selectpath(Fl_Radio_Button*, void*) noexcept;

namespace mcpppp
{
	inline bool dofsb = true, dovmt = true, docim = true, running = false;
	inline int numbuttons = 0;
	inline std::vector<std::pair<bool, std::filesystem::directory_entry>> entries = {};
	inline std::set<std::string> deletedpaths;

	inline std::string getdefaultpath()
	{
#ifdef _WIN32
		const auto wtomb = [](const PWSTR& in)
		{
			const int len = WideCharToMultiByte(CP_UTF8, 0, in, -1, nullptr, 0, nullptr, nullptr);
			std::string out(static_cast<size_t>(len), 0);
			WideCharToMultiByte(CP_UTF8, 0, in, -1, &out.front(), len, nullptr, nullptr);
			while (len > 0 && out.back() == '\0')
			{
				out.pop_back();
			}
			return out;
		};
		PWSTR pwstr;
		SHGetKnownFolderPath(FOLDERID_RoamingAppData, NULL, nullptr, &pwstr);
		std::string str = wtomb(pwstr) + "\\.minecraft\\resourcepacks";
		CoTaskMemFree(pwstr);
		return str;
#elif defined __linux__
		return "~/.minecraft/resourcepacks";
#elif defined __APPLE__ || defined __MACH__
		return "~/Library/Application Support/minecraft/resourcepacks";
#else
		return nullptr;
#endif
	}

	inline void guirun()
		try
	{
		out(3) << "Conversion Started" << std::endl;
		bool valid = false;
		for (const std::pair<bool, std::filesystem::directory_entry>& p : entries)
		{
			if (!p.first)
			{
				continue;
			}
			if (p.second.is_directory())
			{
				valid = true;
				bool success = false;
				if (dofsb)
				{
					if (fsb(p.second.path().u8string(), p.second.path().filename().u8string()).success)
					{
						success = true;
					}
				}
				if (dovmt)
				{
					if (vmt(p.second.path().u8string(), p.second.path().filename().u8string()).success)
					{
						success = true;
					}
				}
				if (docim)
				{
					if (cim(p.second.path().u8string(), p.second.path().filename().u8string()).success)
					{
						success = true;
					}
				}
				if (success)
				{
					mcpppp::checkpackver(p.second);
				}
			}
			else if (p.second.path().extension() == ".zip")
			{
				valid = true;
				bool success = false;
				Zippy::ZipArchive zipa;
				unzip(p.second, zipa);
				std::string folder = p.second.path().stem().u8string();
				if (dofsb)
				{
					if (fsb("mcpppp-temp/" + folder, folder).success)
					{
						success = true;
					}
				}
				if (dovmt)
				{
					if (vmt("mcpppp-temp/" + folder, folder).success)
					{
						success = true;
					}
				}
				if (docim)
				{
					if (cim("mcpppp-temp/" + folder, folder).success)
					{
						success = true;
					}
				}
				if (success)
				{
					mcpppp::checkpackver("mcpppp-temp/" + folder);
					rezip(folder, zipa);
				}
			}
		}
		if (!valid)
		{
			out(4) << "No valid path found, running from default directory: " << getdefaultpath() << std::endl;
			for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(getdefaultpath())))
			{
				if (entry.is_directory())
				{
					bool success = false;
					if (fsb(entry.path().u8string(), entry.path().filename().u8string()).success)
					{
						success = true;
					}
					if (vmt(entry.path().u8string(), entry.path().filename().u8string()).success)
					{
						success = true;
					}
					if (cim(entry.path().u8string(), entry.path().filename().u8string()).success)
					{
						success = true;
					}
					if (success)
					{
						mcpppp::checkpackver(entry);
					}
				}
				else if (entry.path().extension() == ".zip")
				{
					bool success = false;
					Zippy::ZipArchive zipa;
					mcpppp::unzip(entry, zipa);
					const std::string folder = entry.path().stem().u8string();
					if (fsb("mcpppp-temp/" + folder, folder).success)
					{
						success = true;
					}
					if (vmt("mcpppp-temp/" + folder, folder).success)
					{
						success = true;
					}
					if (cim("mcpppp-temp/" + folder, folder).success)
					{
						success = true;
					}
					if (success)
					{
						mcpppp::checkpackver("mcpppp-temp/" + folder);
						mcpppp::rezip(folder, zipa);
					}
				}
			}
		}
		running = false;
		out(3) << "Conversion Finished" << std::endl;
	}
	catch (const nlohmann::json::exception& e)
	{
		out(5) << "FATAL JSON ERROR:" << std::endl << e.what() << std::endl;
		std::exit(-1);
	}
	catch (const Zippy::ZipLogicError& e)
	{
		out(5) << "FATAL ZIP LOGIC ERROR" << std::endl << e.what() << std::endl;
		std::exit(-1);
	}
	catch (const Zippy::ZipRuntimeError& e)
	{
		out(5) << "FATAL ZIP RUNTIME ERROR" << std::endl << e.what() << std::endl;
		std::exit(-1);
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		out(5) << "FATAL FILESYSTEM ERROR:" << std::endl << e.what() << std::endl;
		std::exit(-1);
	}
	catch (const std::exception& e)
	{
		out(5) << "FATAL ERROR:" << std::endl << e.what() << std::endl;
		std::exit(-1);
	}
	catch (...)
	{
		out(5) << "UNKNOWN FATAL ERROR" << std::endl;
		std::exit(-1);
	}

#ifdef _WIN32
	static UINT win_getscale() noexcept
	{
		SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
		return GetDpiForSystem();
	}
#endif

	inline double getscale()
	{
#ifdef _WIN32
		std::future<UINT> dpi = std::async(&win_getscale);
		return dpi.get() / 96.0;
#else
		return 1;
#endif
}

	// add resourcepack to checklist
	inline void addpack(const std::string& name, bool selected)
	{
		// only w is used
		int w = 0, h = 0, dx = 0, dy = 0;
		fl_text_extents(name.c_str(), dx, dy, w, h);
		w = std::lround(w / getscale());
		std::unique_ptr<Fl_Check_Button> o = std::make_unique<Fl_Check_Button>(445, 60 + 15 * numbuttons, w + 30, 15);
		std::unique_ptr<int> temp = std::make_unique<int>(numbuttons);
		o->copy_label(name.c_str());
		o->copy_tooltip(name.c_str());
		o->down_box(FL_DOWN_BOX);
		o->value(static_cast<int>(selected));
		o->user_data(temp.get());
		o->callback(reinterpret_cast<Fl_Callback*>(resourcepack));
		o->when(FL_WHEN_CHANGED);
		ui->scroll->add(o.get());
		o.release();
		temp.release();
		numbuttons++;
	}

	// update paths from "Edit Paths" to path_input
	inline void updatepaths()
	{
		std::string pstr;
		for (const std::string& str : paths)
		{
			pstr += str + " // ";
		}
		if (!paths.empty())
		{
			pstr.erase(pstr.end() - 4, pstr.end()); // erase the last " // "
		}
		ui->path_input->value(pstr.c_str());
	}

	// update settings in "Settings"
	inline void updatesettings()
	{
		ui->autodeletetemptrue->value(static_cast<int>(autodeletetemp));
		ui->autodeletetempfalse->value(static_cast<int>(!autodeletetemp));
		if (dolog)
		{
			ui->log->value(logfilename.c_str());
		}
		ui->timestamptrue->value(static_cast<int>(dotimestamp));
		ui->timestampfalse->value(static_cast<int>(!dotimestamp));
		ui->outputlevelslider->value(outputlevel);
		ui->loglevel->value(loglevel);
		ui->autoreconverttrue->value(static_cast<int>(autoreconvert));
		ui->autoreconvertfalse->value(static_cast<int>(!autoreconvert));
	}

	// update config file to include paths
	inline void updatepathconfig()
	{
		std::set<std::string> temppaths = paths;

		// input stuff from file
		std::ifstream configfile("mcpppp-config.json");
		std::string temp;
		nlohmann::json j;
		try
		{
			temp.resize(std::filesystem::file_size("mcpppp-config.json"));
			configfile.read(&temp.at(0), static_cast<std::streamsize>(std::filesystem::file_size("mcpppp-config.json")));
			j = nlohmann::json::parse(temp, nullptr, true, true);
		}
		catch (const nlohmann::json::exception& e)
		{
			out(5) << e.what() << std::endl;
			throw e;
		}
		configfile.close();

		// remove excess deleted paths
		if (j.contains("gui"))
		{
			if (j["gui"].type() == nlohmann::json::value_t::object)
			{
				if (j["gui"].contains("paths"))
				{
					if (j["gui"]["paths"].type() == nlohmann::json::value_t::array)
					{
						for (const std::string& path : j["gui"]["paths"].get<std::vector<std::string>>())
						{
							if (deletedpaths.find(path) != deletedpaths.end()) // we don't need to delete paths which are in gui (we can override)
							{
								deletedpaths.erase(path);
							}
						}
					}
				}
			}
		}

		// remove excess paths
		if (j.contains("paths"))
		{
			if (j["paths"].type() == nlohmann::json::value_t::array)
			{
				for (const std::string& path : j["paths"].get<std::vector<std::string>>())
				{
					if (temppaths.find(path) != temppaths.end()) // we only need to add paths which haven't already been added (outside of gui)
					{
						temppaths.erase(path);
					}
				}
			}
		}

		config["gui"]["paths"] = std::vector<std::string>(temppaths.begin(), temppaths.end());
		config["gui"]["excludepaths"] = std::vector<std::string>(deletedpaths.begin(), deletedpaths.end());
		std::ofstream fout("mcpppp-config.json");
		fout << "// Please check out the Documentation for the config file before editing it yourself: https://github.com/supsm/MCPPPP/blob/master/CONFIG.md" << std::endl;
		fout << config.dump(1, '\t') << std::endl;
		fout.close();
	}

	// add paths to "Edit Paths" from paths
	inline void addpaths()
	{
		// only w, maxsize, and i are used
		int i = 0, dx = 0, dy = 0, w = 0, h = 0, maxsize = 0;
		ui->paths->clear();
		// make sure all boxes are same size
		for (const std::string& str : paths)
		{
			fl_text_extents(("    " + str).c_str(), dx, dy, w, h);
			w = std::lround(w / getscale());
			maxsize = std::max(maxsize, w);
		}
		for (const std::string& str : paths)
		{
			std::unique_ptr<Fl_Radio_Button> o = std::make_unique<Fl_Radio_Button>(10, 15 + 15 * i, std::max(maxsize, 250), 15);
			o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
			o->copy_label(("    " + str).c_str());
			o->callback(reinterpret_cast<Fl_Callback*>(selectpath));
			ui->paths->add(o.get());
			o.release();

			i++;
		}
	}

	// add path to "Edit Paths" and paths
	inline void addpath(const std::string& name)
	{
		if (paths.find(name) != paths.end())
		{
			return;
		}

		// if name does not end in .minecraft/resourcepacks
		const size_t find1 = name.rfind(".minecraft/resourcepacks");
		const size_t find2 = name.rfind(".minecraft\\resourcepacks");
		if (!ui->dontshowwarning->value() && (find1 < name.size() - 24 || find1 == std::string::npos) && (find2 < name.size() - 24 || find2 == std::string::npos))
		{
			// don't let user do anything until they close window lol
			ui->path_warning->set_modal();
			ui->path_warning->show();
			while (ui->path_warning->shown())
			{
				Fl::wait();
			}
		}

		paths.insert(name);
		addpaths();
	}

#ifdef _WIN32
	inline std::string winfilebrowser()
	{
		const auto wtomb = [](const LPWSTR& in)
		{
			const int len = WideCharToMultiByte(CP_UTF8, 0, in, -1, nullptr, 0, nullptr, nullptr);
			std::string out(static_cast<size_t>(len), 0);
			WideCharToMultiByte(CP_UTF8, 0, in, -1, &out.front(), len, nullptr, nullptr);
			while (len > 0 && out.back() == '\0')
			{
				out.pop_back();
			}
			return out;
		};
		LPWSTR path = nullptr;
		std::string str;
		IFileDialog* pfd = nullptr;
		if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
		{
			DWORD dwOptions = 0;
			if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
			{
				pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
			}
			if (SUCCEEDED(pfd->Show(nullptr)))
			{
				IShellItem* psi = nullptr;
				if (SUCCEEDED(pfd->GetResult(&psi)))
				{
					psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &path);
					psi->Release();
					str = wtomb(path);
					CoTaskMemFree(path);
				}
			}
			pfd->Release();
		}
		return str;
	}
#endif
}
#endif
