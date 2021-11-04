/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef GUI
#include "fsb.h"
#include "vmt.h"
#include "cim.h"

#ifdef _WIN32
#define WIN32 // fltk wants this for some reason
#endif

static bool dofsb = true, dovmt = true, docim = true, running = false;
static int numbuttons = 0;
inline std::vector<std::pair<bool, std::filesystem::directory_entry>> entries = {};
static std::set<std::string> deletedpaths;
static std::unique_ptr<Fl_Widget> selectedwidget;

void addpack(const std::string& name, bool selected);
void addpath(const std::string& name);
void addpaths();
void updatepaths();
void updatepathconfig();
std::string winfilebrowser();

inline void guirun()
try
{
	for (const std::pair<bool, std::filesystem::directory_entry>& p : entries)
	{
		if (!p.first)
		{
			continue;
		}
		if (p.second.is_directory())
		{
			if (dofsb)
			{
				fsb(p.second.path().u8string(), p.second.path().filename().u8string());
			}
			if (dovmt)
			{
				vmt(p.second.path().u8string(), p.second.path().filename().u8string());
			}
			if (docim)
			{
				cim(p.second.path().u8string(), p.second.path().filename().u8string());
			}
		}
		else if (p.second.path().extension() == ".zip")
		{
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
				rezip(folder, zipa);
			}
		}
	}
	running = false;
	out(3) << "All Done!" << std::endl;
}
catch (const nlohmann::json::exception& e)
{
	out(5) << "FATAL JSON ERROR:" << std::endl << e.what() << std::endl;
}
catch (const Zippy::ZipLogicError& e)
{
	out(5) << "FATAL ZIP LOGIC ERROR" << std::endl << e.what() << std::endl;
}
catch (const Zippy::ZipRuntimeError& e)
{
	out(5) << "FATAL ZIP RUNTIME ERROR" << std::endl << e.what() << std::endl;
}
catch (const std::filesystem::filesystem_error& e)
{
	out(5) << "FATAL FILESYSTEM ERROR:" << std::endl << e.what() << std::endl;
}
catch (const std::exception& e)
{
	out(5) << "FATAL ERROR:" << std::endl << e.what() << std::endl;
}
catch (...)
{
	out(5) << "UNKNOWN FATAL ERROR" << std::endl;
}

// callback for run button
inline void run(Fl_Button* o, void* v)
{
	if (running)
	{
		return;
	}
	running = true;
	std::thread t(guirun);
	t.detach();
}

// callback for "CIM", "VMT", "FSB" checkboxes
void conversion(Fl_Check_Button* o, void* v) // NOLINT
{
	if (strcmp(o->label(), "FSB") == 0)
	{
		dofsb = static_cast<bool>(o->value());
	}
	if (strcmp(o->label(), "VMT") == 0)
	{
		dovmt = static_cast<bool>(o->value());
	}
	if (strcmp(o->label(), "CIM") == 0)
	{
		docim = static_cast<bool>(o->value());
	}
	std::cout << o->label() << static_cast<int>(o->value()) << std::endl;
}

// callback for resourcepack checkboxes
inline void resourcepack(Fl_Check_Button* o, void* v)
{
	entries.at(static_cast<size_t>(*(static_cast<int*>(v)))).first = static_cast<bool>(o->value());
	ui->allpacks->value(0);
	std::cout << o->label() << " " << *static_cast<int*>(v) << " " << static_cast<int>(o->value()) << std::endl;
}

// callback for browse button
inline void browse(Fl_Button* o, void* v) // NOLINT
{
	ui->edit_paths->show();
}

// callback for reload button
inline void reload(Fl_Button* o, void* v) // NOLINT
{
	entries.clear();
	ui->scroll->clear();
	numbuttons = 0;
	ui->scroll->begin(); // padding
	std::unique_ptr<Fl_Check_Button> pad = std::make_unique<Fl_Check_Button>(470, 45, 150, 15);
	pad->down_box(FL_DOWN_BOX);
	pad->labeltype(FL_NO_LABEL);
	pad->hide();
	pad->deactivate();
	ui->scroll->end();
	for (const std::string& path : paths)
	{
		if (std::filesystem::is_directory(std::filesystem::u8path(path)))
		{
			for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(path)))
			{
				if (entry.is_directory() || entry.path().extension() == ".zip")
				{
					entries.emplace_back(std::make_pair(true, entry));
					addpack(entry.path().filename().u8string(), true);
					std::cout << entry.path().filename().u8string() << std::endl;
				}
			}
		}
	}
	ui->allpacks->value(1);
	Fl::wait();
}

// callback for path_input
inline void editpath(Fl_Input* o, void* v)
{
	deletedpaths.insert(paths.begin(), paths.end());
	paths.clear();
	std::string str = o->value();
	while (str.find(" // ") != std::string::npos)
	{
		const size_t i = str.find(" // ");
		paths.insert(std::string(str.begin(), str.begin() + static_cast<std::string::difference_type>(i)));
		deletedpaths.erase(std::string(str.begin(), str.begin() + static_cast<std::string::difference_type>(i)));
		str.erase(str.begin(), str.begin() + static_cast<std::string::difference_type>(i + 4));
	}
	if (!str.empty())
	{
		paths.insert(str);
		deletedpaths.erase(str);
	}
	addpaths();
	updatepathconfig();
}

// callback for "Add" button in "Edit Paths"
inline void addrespath(Fl_Button* o, void* v)
{
	std::string str;
#ifdef _WIN32
	str = winfilebrowser();
#else
	std::unique_ptr<Fl_Native_File_Chooser> chooser = std::make_unique<Fl_Native_File_Chooser>(1); // browse directory
	str = chooser->show();
#endif
	if (!str.empty() && paths.find(str) == paths.end())
	{
		addpath(str);
		deletedpaths.erase(str);
		updatepaths();
		updatepathconfig();
		std::cout << str << std::endl;
	}
	ui->edit_paths->redraw();
	reload(nullptr, nullptr);
}

// callback for "Delete" button in "Edit Paths"
inline void deleterespath(Fl_Button* o, void* v)
{
	if (selectedwidget == nullptr)
	{
		return;
	}
	paths.erase(selectedwidget->label());
	deletedpaths.insert(selectedwidget->label());
	selectedwidget.reset();
	addpaths();
	updatepaths();
	ui->paths->hide();
	ui->paths->show();
	updatepathconfig();
	ui->edit_paths->redraw();
	reload(nullptr, nullptr);
}

// callback for paths buttons in "Edit Paths"
inline void selectpath(Fl_Radio_Button* o, void* v) noexcept
{
	selectedwidget.reset(o);
}

// callback for settings button
inline void opensettings(Fl_Button* o, void* v)
{
	ui->savewarning->hide();
	ui->settings->show();
	ui->box1->redraw(); // the outlining boxes disappear for some reason
	ui->box2->redraw();
}

// callback for help button
inline void openhelp(Fl_Button* o, void* v)
{
	ui->help->show();
	ui->box1->redraw();
	ui->box2->redraw();
}

// callback for save button in setings
inline void savesettings(Fl_Button* o, void* v)
{
	logfilename = ui->log->value();
	dolog = (logfilename.empty());
	if (dolog)
	{
		logfile.close();
		logfile.open(logfilename);
	}
	dotimestamp = static_cast<bool>(ui->timestamptrue->value());
	outputlevel = static_cast<int>(ui->outputlevel->value());
	loglevel = static_cast<int>(ui->loglevel->value());
	autoreconvert = static_cast<bool>(ui->autoreconverttrue->value());

	config["gui"]["settings"]["log"] = logfilename;
	config["gui"]["settings"]["timestamp"] = dotimestamp;
	config["gui"]["settings"]["outputLevel"] = outputlevel;
	config["gui"]["settings"]["logLevel"] = loglevel;
	config["gui"]["settings"]["autoReconvert"] = autoreconvert;

	// remove excess settings
	std::vector<std::string> toremove;
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
	if (j.contains("settings"))
	{
		if (j["settings"].type() == nlohmann::json::value_t::object)
		{
			for (const auto& setting : j["settings"].items())
			{
				for (const auto& setting2 : config["gui"]["settings"].items())
				{
					if (lowercase(setting.key()) == lowercase(setting2.key()))
					{
						if (setting.value() == nlohmann::json(setting2.value()))
						{
							config["gui"]["settings"].erase(setting2.key());
							break;
						}
					}
				}
			}
		}
	}
	for (const auto& setting : config["gui"]["settings"].items())
	{
		if (std::get<2>(settings.at(lowercase(setting.key()))) == nlohmann::json(setting.value()))
		{
			toremove.push_back(setting.key());
		}
	}
	for (const std::string& s : toremove)
	{
		config["gui"]["settings"].erase(s);
	}

	std::ofstream fout("mcpppp-config.json");
	fout << "// Please check out the Documentation for the config file before editing it yourself: https://github.com/supsm/MCPPPP/blob/master/CONFIG.md" << std::endl;
	fout << config.dump(1, '\t') << std::endl;
	fout.close();

	ui->savewarning->hide();
}

// callback for edited settings
inline void settingchanged(Fl_Widget* o, void* v)
{
	ui->savewarning->show();
}

// callback for select all/none
inline void selectall(Fl_Check_Button* o, void* v)
{
	ui->scroll->clear();
	numbuttons = 0;
	// padding
	std::unique_ptr<Fl_Check_Button> pad = std::make_unique<Fl_Check_Button>(470, 45, 150, 15);
	pad->down_box(FL_DOWN_BOX);
	pad->labeltype(FL_NO_LABEL);
	pad->hide();
	pad->deactivate();
	ui->scroll->add(pad.get());
	pad.release();
	for (auto& entry : entries)
	{
		addpack(entry.second.path().filename().u8string(), static_cast<bool>(o->value()));
		entry.first = static_cast<bool>(o->value());
	}
	ui->scroll->redraw();
	Fl::wait();
}

// callback for delete mcpppp-temp
inline void deletetemp(Fl_Button* o, void* v)
{
	std::filesystem::remove_all("mcpppp-temp");
	ui->tempfound->hide();
}

// callback for not deleting mcpppp-temp
inline void dontdeletetemp(Fl_Button* o, void* v)
{
	out(5) << "Folder named \"mcpppp-temp\" found. Please remove this folder." << std::endl;
	running = true;
	ui->tempfound->hide();
}

// add resourcepack to checklist
inline void addpack(const std::string& name, bool selected)
{
	int w = 0, h = 0, dx = 0, dy = 0;
	fl_text_extents(name.c_str(), dx, dy, w, h);
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
	if (dolog)
	{
		ui->log->value(logfilename.c_str());
	}
	ui->timestamptrue->value(static_cast<int>(dotimestamp));
	ui->timestampfalse->value(static_cast<int>(!dotimestamp));
	ui->outputlevel->value(outputlevel);
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
	int i = 0, dx = 0, dy = 0, w = 0, h = 0;
	ui->paths->clear();
	for (const std::string& str : paths)
	{
		fl_text_extents(str.c_str(), dx, dy, w, h);
		std::unique_ptr<Fl_Radio_Button> o = std::make_unique<Fl_Radio_Button>(10, 15 + 15 * i, std::max(w + 30, 250), 15);
		o->copy_label(str.c_str());
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
	int w, h, dx, dy;
	fl_text_extents(name.c_str(), dx, dy, w, h);
	std::unique_ptr<Fl_Radio_Button> o = std::make_unique<Fl_Radio_Button>(10, 15 + 15 * paths.size(), std::max(w + 30, 250), 15);
	o->copy_label(name.c_str());
	o->callback(reinterpret_cast<Fl_Callback*>(selectpath));
	ui->paths->add(o.get());
	o.release();
	paths.insert(name);
}

#ifdef _WIN32
inline std::string winfilebrowser()
{
	LPWSTR path = nullptr;
	std::wstring str;
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
				str = path;
				CoTaskMemFree(path);
			}
		}
		pfd->Release();
	}
	return wtomb(str);
}
#endif
#endif
