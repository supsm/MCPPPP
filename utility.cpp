#include <algorithm>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "json.hpp"
#include "lodepng.cpp"

#ifdef _WIN32
#include <direct.h> //zippy wants this
#endif

#include "Zippy.hpp"

#ifdef GUI

#ifdef _WIN32
#define WIN32 // fltk wants this for some reason
#endif

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Radio_Button.H>

#include "mcpppp.cxx" // ui class

#endif

inline bool autodeletetemp = false, pauseonexit = true, dolog = true, dotimestamp = false, autoreconvert = false;
inline int outputlevel = 3, loglevel = 1;
inline std::ofstream logfile("mcpppp-log.txt");
std::string logfilename = "mcpppp-log.txt";

inline std::set<std::string> paths = {};
inline nlohmann::ordered_json config;

enum class type { boolean, integer, string };

// type, pointer to variable to modify, default value
std::unordered_map<std::string, std::tuple<type, void*, nlohmann::json>> settings =
{
	{"pauseonexit", {type::boolean, &pauseonexit, pauseonexit}},
	{"log", {type::string, &logfilename, logfilename}},
	{"timestamp", {type::boolean, &dotimestamp, dotimestamp}},
	{"autodeletetemp", {type::boolean, &autodeletetemp, autodeletetemp}},
	{"outputlevel", {type::integer, &outputlevel, outputlevel}},
	{"loglevel", {type::integer, &loglevel, loglevel}},
	{"autoreconvert", {type::boolean, &autoreconvert, autoreconvert}}
};

#ifdef GUI
bool dofsb = true, dovmt = true, docim = true, running = false;
long long numbuttons = 0;
std::stringstream ss;
inline std::vector<std::pair<bool, std::filesystem::directory_entry>> entries = {};
std::set<std::string> deletedpaths;
Fl_Text_Buffer textbuffer;
Fl_Widget* selectedwidget;
inline UI* ui = new UI();
#endif

std::string lowercase(std::string str)
{
	for (char& c : str)
	{
		if (c >= 'A' && c <= 'Z')
		{
			c += 32;
		}
	}
	return str;
}

std::string timestamp()
{
	time_t timet = time(NULL);
	struct tm* timeinfo = localtime(&timet);
	std::string hour = std::to_string(timeinfo->tm_hour);
	if (hour.length() == 1)
	{
		hour.insert(hour.begin(), '0');
	}
	std::string min = std::to_string(timeinfo->tm_min);
	if (min.length() == 1)
	{
		min.insert(min.begin(), '0');
	}
	std::string sec = std::to_string(timeinfo->tm_sec);
	if (sec.length() == 1)
	{
		sec.insert(sec.begin(), '0');
	}
	return '[' + hour + ':' + min + ':' + sec + "] ";
}

std::string ununderscore(std::string str)
{
	std::string str2;
	for (char& c : str)
	{
		if (c != '_')
		{
			str2 += c;
		}
	}
	return str2;
}

namespace supsm
{
	void copy(std::filesystem::path from, std::filesystem::path to)
	{
		if (std::filesystem::is_directory(to))
		{
			return;
		}
		if (std::filesystem::exists(to))
		{
			std::filesystem::remove(to);
		}
		std::filesystem::create_directories(to.parent_path());
		std::filesystem::copy(from, to);
	}
}

std::string wtomb(std::wstring str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(str);
}

void findreplace(std::string& source, std::string find, std::string replace)
{
	int pos;
	while (source.find(find) != std::string::npos)
	{
		pos = source.find(find);
		source.replace(pos, find.length(), replace);
	}
}

bool c, file;

class outstream
{
public:
	bool first = false;
	template<typename T>
	outstream operator<<(T& value)
	{
		if (c)
		{
#ifdef GUI
			if (first)
			{
				ss << (dotimestamp ? timestamp() : "");
			}
			ss << value;
			textbuffer.text(ss.str().c_str());
			ui->text_display->buffer(textbuffer);
			Fl::wait();
#else
			if (first)
			{
				std::cout << (dotimestamp ? timestamp() : "");
			}
			std::cout << value;
#endif
		}
		if (file && logfile.good())
		{
			if (first)
			{
				logfile << timestamp();
			}
			logfile << value;
		}
		return outstream();
	}
	outstream operator<<(std::string str)
	{
		if (c)
		{
#ifdef GUI
			if (first)
			{
				ss << (dotimestamp ? timestamp() : "");
			}
			ss << str;
			textbuffer.text(ss.str().c_str());
			ui->text_display->buffer(textbuffer);
			Fl::wait();
#else
			if (first)
			{
				std::cout << (dotimestamp ? timestamp() : "");
			}
			std::cout << str;
#endif
		}
		if (file && logfile.good())
		{
			if (first)
			{
				logfile << timestamp();
			}
			logfile << str;
		}
		return outstream();
	}
	outstream operator<<(std::ostream& (*f)(std::ostream&))
	{
		if (c)
		{
#ifdef GUI
			if (first)
			{
				ss << (dotimestamp ? timestamp() : "");
			}
			ss << f;
			textbuffer.text(ss.str().c_str());
			ui->text_display->buffer(textbuffer);
			Fl::wait();
#else
			if (first)
			{
				std::cout << (dotimestamp ? timestamp() : "");
			}
			std::cout << f;
#endif
		}
		if (file && logfile.good())
		{
			if (first)
			{
				logfile << timestamp();
			}
			logfile << f;
		}
		return outstream();
	}
};

outstream out(int level)
{
	outstream o;
	o.first = true;
	if (level >= outputlevel)
	{
		c = true;
	}
	else
	{
		c = false;
	}
	if (level >= loglevel)
	{
		file = true;
	}
	else
	{
		file = false;
	}
	return o;
}

void setting(std::string option, nlohmann::json j)
{
	if (settings.find(lowercase(option)) == settings.end())
	{
		out(4) << "Unknown setting: " << option << std::endl;
		return;
	}
	type t = std::get<0>(settings[lowercase(option)]);
	void* var = std::get<1>(settings[lowercase(option)]);
	if (t == type::boolean)
	{
		try
		{
			*(bool*)(var) = j.get<bool>();
		}
		catch (nlohmann::json::exception& e)
		{
			out(5) << "Not a valid value for " << option << ": " << j << "; Expected bool" << std::endl;
		}
	}
	else if (t == type::integer)
	{
		try
		{
			*(int*)(var) = j.get<int>();
		}
		catch (nlohmann::json::exception& e)
		{
			out(5) << "Not a valid value for " << option << ": " << j << "; Expected int" << std::endl;
		}
	}
	else
	{
		try
		{
			*(std::string*)(var) = j.get<std::string>();
			if (option == "log")
			{
				dolog = (j.get<std::string>() == "");
				if (dolog)
				{
					logfile.close();
					logfile.open(j.get<std::string>());
				}
			}
		}
		catch (nlohmann::json::exception& e)
		{
			out(5) << "Not a valid value for " << option << ": " << j << "; Expected string" << std::endl;
		}
	}
}

void readconfig()
{
	paths.clear();
	if (!config.contains("settings"))
	{
		out(4) << "No settings found" << std::endl;
	}
	else if (config["settings"].type() != nlohmann::json::value_t::object)
	{
		out(5) << "settings must be an object, got " << config["settings"].type_name() << std::endl;
	}
	else
	{
		for (auto& j : config["settings"].items())
		{
			setting(j.key(), j.value());
		}
	}
	if (!config.contains("paths"))
	{
		out(4) << "No paths found" << std::endl;
		config["paths"] = nlohmann::json::array();
	}
	else if (config["paths"].type() != nlohmann::json::value_t::array)
	{
		out(5) << "paths must be an array, got " << config["paths"].type_name() << std::endl;
		throw std::invalid_argument(std::string("paths must be array, got ") + config["paths"].type_name());
	}
	else
	{
		paths.insert(config["paths"].begin(), config["paths"].end());
	}
	if (config.contains("gui"))
	{
		if (config["gui"].type() == nlohmann::json::value_t::object)
		{
			if (config["gui"].contains("settings"))
			{
				if (config["gui"]["settings"].type() == nlohmann::json::value_t::object)
				{
					for (auto& j : config["gui"]["settings"].items())
					{
						setting(j.key(), j.value());
					}
				}
			}
			if (config["gui"].contains("paths"))
			{
				if (config["gui"]["paths"].type() == nlohmann::json::value_t::array)
				{
					paths.insert(config["gui"]["paths"].begin(), config["gui"]["paths"].end());
				}
			}
			if (config["gui"].contains("excludepaths"))
			{
				if (config["gui"]["excludepaths"].type() == nlohmann::json::value_t::array)
				{
					for (std::string& path : config["gui"]["excludepaths"].get<std::vector<std::string>>())
					{
						paths.erase(path);
					}
				}
			}
		}
	}
}

#ifdef GUI
#include "fsb.cpp"
#include "vmt.cpp"
#include "cim.cpp"

void addpack(std::string, bool selected);
void addpath(std::string);
void addpaths();
void reload(Fl_Button* o, void* v);
void updatepaths();
void updatepathconfig();
std::string winfilebrowser();

void guirun()
{
	for (std::pair<bool, std::filesystem::directory_entry>& p : entries)
	{
		if (p.first)
		{
			if (p.second.is_directory())
			{
				if (dofsb)
				{
					fsb(p.second.path().u8string(), p.second.path().filename().u8string(), false);
				}
				if (dovmt)
				{
					vmt(p.second.path().u8string(), p.second.path().filename().u8string(), false);
				}
				if (docim)
				{
					cim(p.second.path().u8string(), p.second.path().filename().u8string(), false);
				}
			}
			else if (p.second.path().extension() == ".zip")
			{
				if (dofsb)
				{
					fsb(p.second.path().u8string(), p.second.path().filename().u8string(), true);
				}
				if (dovmt)
				{
					vmt(p.second.path().u8string(), p.second.path().filename().u8string(), true);
				}
				if (docim)
				{
					cim(p.second.path().u8string(), p.second.path().filename().u8string(), true);
				}
			}
		}
	}
	running = false;
	out(3) << "All Done!" << std::endl;
}

// callback for run button
void run(Fl_Button* o, void* v)
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
void conversion(Fl_Check_Button* o, void* v)
{
	if (!strcmp(o->label(), "FSB"))
	{
		dofsb = o->value();
	}
	if (!strcmp(o->label(), "VMT"))
	{
		dovmt = o->value();
	}
	if (!strcmp(o->label(), "CIM"))
	{
		docim = o->value();
	}
	std::cout << o->label() << int(o->value()) << std::endl;
}

// callback for resourcepack checkboxes
void resourcepack(Fl_Check_Button* o, void* v)
{
	entries[uintptr_t(v)].first = o->value();
	ui->allpacks->value(0);
	std::cout << o->label() << " " << uintptr_t(v) << " " << int(o->value()) << std::endl;
}

// callback for browse button
void browse(Fl_Button* o, void* v)
{
	ui->edit_paths->show();
}

// callback for reload button
void reload(Fl_Button* o, void* v)
{
	entries.clear();
	ui->scroll->clear();
	numbuttons = 0;
	ui->scroll->begin(); // padding
	Fl_Check_Button* pad = new Fl_Check_Button(470, 45, 150, 15);
	pad->down_box(FL_DOWN_BOX);
	pad->labeltype(FL_NO_LABEL);
	pad->hide();
	pad->deactivate();
	ui->scroll->end();
	for (const std::string& path : paths)
	{
		if (std::filesystem::is_directory(std::filesystem::u8path(path)))
		{
			for (auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(path)))
			{
				if (entry.is_directory() || entry.path().extension() == ".zip")
				{
					entries.push_back(std::make_pair(true, entry));
					addpack(entry.path().filename().u8string(), 1);
					std::cout << entry.path().filename().u8string() << std::endl;
				}
			}
		}
	}
	ui->allpacks->value(1);
	Fl::wait();
}

// callback for path_input
void editpath(Fl_Input* o, void* v)
{
	deletedpaths.insert(paths.begin(), paths.end());
	paths.clear();
	std::string str = o->value();
	while (str.find(" // ") != std::string::npos)
	{
		int i = str.find(" // ");
		paths.insert(std::string(str.begin(), str.begin() + i));
		deletedpaths.erase(std::string(str.begin(), str.begin() + i));
		str.erase(str.begin(), str.begin() + i + 4);
	}
	if (str != "")
	{
		paths.insert(str);
		deletedpaths.erase(str);
	}
	addpaths();
	updatepathconfig();
}

// callback for "Add" button in "Edit Paths"
void addrespath(Fl_Button* o, void* v)
{
	std::string str;
#ifdef _WIN32
	str = winfilebrowser();
#else
	Fl_Native_File_Chooser* chooser = new Fl_Native_File_Chooser(1); // browse directory
	str = chooser->show();
#endif
	if (str != "" && paths.find(str) == paths.end())
	{
		addpath(str);
		deletedpaths.erase(str);
		updatepaths();
		updatepathconfig();
		std::cout << str << std::endl;
	}
	reload(NULL, NULL);
}

// callback for "Delete" button in "Edit Paths"
void deleterespath(Fl_Button* o, void* v)
{
	if (selectedwidget == nullptr)
	{
		return;
	}
	paths.erase(selectedwidget->label());
	deletedpaths.insert(selectedwidget->label());
	delete selectedwidget;
	addpaths();
	updatepaths();
	ui->paths->hide();
	ui->paths->show();
	updatepathconfig();
	reload(NULL, NULL);
}

// callback for paths buttons in "Edit Paths"
void selectpath(Fl_Radio_Button* o, void* v)
{
	selectedwidget = o;
}

// callback for settings button
void opensettings(Fl_Button* o, void* v)
{
	ui->savewarning->hide();
	ui->settings->show();
	ui->box1->redraw(); // the outlining boxes disappear for some reason
	ui->box2->redraw();
}

// callback for help button
void openhelp(Fl_Button* o, void* v)
{
	ui->help->show();
	ui->box1->redraw();
	ui->box2->redraw();
}

// callback for save button in setings
void savesettings(Fl_Button* o, void* v)
{
	logfilename = ui->log->value();
	dolog = (logfilename == "");
	if (dolog)
	{
		logfile.close();
		logfile.open(logfilename);
	}
	dotimestamp = ui->timestamptrue->value();
	outputlevel = ui->outputlevel->value();
	loglevel = ui->loglevel->value();
	autoreconvert = ui->autoreconverttrue->value();

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
		configfile.read((char*)(temp.c_str()), std::filesystem::file_size("mcpppp-config.json"));
		j = nlohmann::json::parse(temp, nullptr, true, true);
	}
	catch (nlohmann::json::exception& e)
	{
		out(5) << e.what() << std::endl;
		throw e;
	}
	configfile.close();
	if (j.contains("settings"))
	{
		if (j["settings"].type() == nlohmann::json::value_t::object)
		{
			for (auto& setting : j["settings"].items())
			{
				for (auto& setting2 : config["gui"]["settings"].items())
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
	for (auto& setting : config["gui"]["settings"].items())
	{
		if (std::get<2>(settings[lowercase(setting.key())]) == nlohmann::json(setting.value()))
		{
			toremove.push_back(setting.key());
		}
	}
	for (std::string& s : toremove)
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
void settingchanged(Fl_Widget* o, void* v)
{
	ui->savewarning->show();
}

// callback for select all/none
void selectall(Fl_Check_Button* o, void* v)
{
	ui->scroll->clear();
	numbuttons = 0;
	ui->scroll->begin(); // padding
	Fl_Check_Button* pad = new Fl_Check_Button(470, 45, 150, 15);
	pad->down_box(FL_DOWN_BOX);
	pad->labeltype(FL_NO_LABEL);
	pad->hide();
	pad->deactivate();
	ui->scroll->end();
	for (auto& entry : entries)
	{
		addpack(entry.second.path().filename().u8string(), o->value());
	}
	Fl::wait();
}

// callback for delete mcpppp-temp
void deletetemp(Fl_Button* o, void* v)
{
	std::filesystem::remove_all("mcpppp-temp");
	ui->tempfound->hide();
}

// callback for not deleting mcpppp-temp
void dontdeletetemp(Fl_Button* o, void* v)
{
	out(5) << "Folder named \"mcpppp-temp\" found. Please remove this folder." << std::endl;
	running = true;
	ui->tempfound->hide();
}

// add resourcepack to checklist
void addpack(std::string name, bool selected)
{
	int w, h, dx, dy;
	ui->scroll->begin();
	fl_text_extents(name.c_str(), dx, dy, w, h);
	Fl_Check_Button* o = new Fl_Check_Button(445, 60 + 15 * numbuttons, w + 30, 15);
	o->copy_label(name.c_str());
	o->down_box(FL_DOWN_BOX);
	o->value(selected);
	o->user_data((void*)(numbuttons));
	o->callback((Fl_Callback*)(resourcepack));
	o->when(FL_WHEN_CHANGED);
	ui->scroll->end();
	numbuttons++;
}

// update paths from "Edit Paths" to path_input
void updatepaths()
{
	std::string pstr;
	for (const std::string& str : paths)
	{
		pstr += str + " // ";
	}
	if (paths.size())
	{
		pstr.erase(pstr.end() - 4, pstr.end()); // erase the last " // "
	}
	ui->path_input->value(pstr.c_str());
}

// update settings in "Settings"
void updatesettings()
{
	if (dolog)
	{
		ui->log->value(logfilename.c_str());
	}
	ui->timestamptrue->value(dotimestamp);
	ui->timestampfalse->value(!dotimestamp);
	ui->outputlevel->value(outputlevel);
	ui->loglevel->value(loglevel);
	ui->autoreconverttrue->value(autoreconvert);
	ui->autoreconvertfalse->value(!autoreconvert);
}

// update config file to include paths
void updatepathconfig()
{
	std::set<std::string> temppaths = paths;

	// input stuff from file
	std::ifstream configfile("mcpppp-config.json");
	std::string temp;
	nlohmann::json j;
	try
	{
		temp.resize(std::filesystem::file_size("mcpppp-config.json"));
		configfile.read((char*)(temp.c_str()), std::filesystem::file_size("mcpppp-config.json"));
		j = nlohmann::json::parse(temp, nullptr, true, true);
	}
	catch (nlohmann::json::exception& e)
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
					for (std::string& path : j["gui"]["paths"].get<std::vector<std::string>>())
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
			for (std::string& path : j["paths"].get<std::vector<std::string>>())
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
void addpaths()
{
	int i = 0, dx, dy, w, h;
	ui->paths->clear();
	ui->paths->begin();
	for (const std::string& str : paths)
	{
		fl_text_extents(str.c_str(), dx, dy, w, h);
		Fl_Radio_Button* o = new Fl_Radio_Button(10, 15 + 15 * i, std::max(w + 30, 250), 15);
		o->copy_label(str.c_str());
		o->callback((Fl_Callback*)(selectpath));

		i++;
	}
	ui->paths->end();
}

// add path to "Edit Paths" and paths
void addpath(std::string name)
{
	if (paths.find(name) != paths.end())
	{
		return;
	}
	int w, h, dx, dy;
	ui->paths->begin();
	fl_text_extents(name.c_str(), dx, dy, w, h);
	Fl_Radio_Button* o = new Fl_Radio_Button(10, 15 + 15 * paths.size(), std::max(w + 30, 250), 15);
	o->copy_label(name.c_str());
	o->callback((Fl_Callback*)(selectpath));
	ui->scroll->end();
	paths.insert(name);
}

#ifdef _WIN32
std::string winfilebrowser()
{
	LPWSTR path;
	std::wstring str;
	IFileDialog* pfd;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
	{
		DWORD dwOptions;
		if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
		{
			pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
		}
		if (SUCCEEDED(pfd->Show(NULL)))
		{
			IShellItem* psi;
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
