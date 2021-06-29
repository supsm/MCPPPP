#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
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

extern bool autodeletetemp = false, pauseonexit = true, dolog = false, dotimestamp = false, deletesource = false;
extern int outputlevel = 3, loglevel = 2;
extern std::ofstream logfile("log.txt");
extern std::set<std::string> paths = {};
#ifdef GUI
bool dofsb = true, dovmt = true, docim = true, running = false;
int numbuttons = 0;
std::stringstream ss;
extern std::vector<std::pair<bool, std::filesystem::directory_entry>> entries = {};
std::set<std::string> deletedpaths;
Fl_Text_Buffer textbuffer;
Fl_Widget* selectedwidget;
extern UI* ui = new UI();
#endif

std::string lowercase(std::string str)
{
	for (int i = 0; i < str.size(); i++)
	{
		if (str[i] >= 'A' && str[i] <= 'Z')
		{
			str[i] += 32;
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
	for (int i = 0; i < str.size(); i++)
	{
		if (str[i] != '_')
		{
			str2 += str[i];
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

void setting(std::string option, std::string value)
{
	if (lowercase(option) == "pauseonexit")
	{
		if (lowercase(value) == "true")
		{
			pauseonexit = true;
		}
		else if (lowercase(value) == "false")
		{
			pauseonexit = false;
		}
		else
		{
			std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid value for " << option << ": " << value << " Expected true, false" << std::endl;
		}
	}
	else if (lowercase(option) == "log")
	{
		dolog = true;
		logfile.open(value);
	}
	else if (lowercase(option) == "timestamp")
	{
		if (lowercase(value) == "true")
		{
			dotimestamp = true;
		}
		else if (lowercase(value) == "false")
		{
			dotimestamp = false;
		}
		else
		{
			std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid value for " << option << ": " << value << " Expected true, false" << std::endl;
		}
	}
	else if (lowercase(option) == "autodeletetemp")
	{
		if (lowercase(value) == "true")
		{
			autodeletetemp = true;
		}
		else if (lowercase(value) == "false")
		{
			autodeletetemp = false;
		}
		else
		{
			std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid value for " << option << ": " << value << " Expected true, false" << std::endl;
		}
	}
	else if (lowercase(option) == "outputlevel")
	{
		try
		{
			outputlevel = stoi(value);
		}
		catch (std::exception e)
		{
			std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid value for " << option << ": " << value << " Expected integer, 1-5" << std::endl;
		}
	}
	else if (lowercase(option) == "loglevel")
	{
		try
		{
			loglevel = stoi(value);
		}
		catch (std::exception e)
		{
			std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid value for " << option << ": " << value << " Expected integer, 1-5" << std::endl;
		}
	}
	else if (lowercase(option) == "deletesource")
	{
		if (lowercase(value) == "true")
		{
			deletesource = true;
		}
		else if (lowercase(value) == "false")
		{
			deletesource = false;
		}
		else
		{
			std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid value for " << option << ": " << value << " Expected true, false" << std::endl;
		}
	}
	else
	{
		std::cerr << (dotimestamp ? timestamp() : "") << "Not a valid option: " << option << std::endl;
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

#ifdef GUI
#include "fsb.cpp"
#include "vmt.cpp"
#include "cim.cpp"

void addpack(std::string);
void addpath(std::string);
void updatepaths();

// callback for run button
void run(Fl_Button* o, void* v)
{
	if (running)
	{
		return;
	}
	running = true;
	for (int i = 0; i < entries.size(); i++)
	{
		if (entries[i].first)
		{
			if (entries[i].second.is_directory())
			{
				if (dofsb)
				{
					fsb(entries[i].second.path().u8string(), entries[i].second.path().filename().u8string(), false);
				}
				if (dovmt)
				{
					vmt(entries[i].second.path().u8string(), entries[i].second.path().filename().u8string(), false);
				}
				if (docim)
				{
					cim(entries[i].second.path().u8string(), entries[i].second.path().filename().u8string(), false);
				}
			}
			else if (entries[i].second.path().extension() == ".zip")
			{
				if (dofsb)
				{
					fsb(entries[i].second.path().u8string(), entries[i].second.path().filename().u8string(), true);
				}
				if (dovmt)
				{
					vmt(entries[i].second.path().u8string(), entries[i].second.path().filename().u8string(), true);
				}
				if (docim)
				{
					cim(entries[i].second.path().u8string(), entries[i].second.path().filename().u8string(), true);
				}
			}
		}
	}
	running = false;
	out(3) << "All Done!" << std::endl;
	std::cout << "run\n";
}

// callback for "CIM", "VMT", "FSB" checkboxes
void conversion(Fl_Check_Button* o, void* v)
{
	if (o->label() == "FSB")
	{
		dofsb = o->value();
	}
	if (o->label() == "VMT")
	{
		dovmt = o->value();
	}
	if (o->label() == "CIM")
	{
		docim = o->value();
	}
	std::cout << o->label() << int(o->value()) << std::endl;
}

// callback for resourcepack checkboxes
void resourcepack(Fl_Check_Button* o, void* v)
{
	entries[int(v)].first = o->value();
	std::cout << o->label() << " " << int(v) << " " << int(o->value()) << std::endl;
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
	for (std::string path : paths)
	{
		for (auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(path)))
		{
			if (entry.is_directory() || entry.path().extension() == ".zip")
			{
				entries.push_back(std::make_pair(true, entry));
				addpack(entry.path().filename().u8string());
				std::cout << entry.path().filename().u8string() << std::endl;
			}
		}
	}
	Fl::wait();
}

// callback for path_input
void editpath(Fl_Input* o, void* v)
{
	// TODO: parse the inputted path
}

// callback for "Add" button in "Edit Paths"
void addrespath(Fl_Button* o, void* v)
{
	Fl_Native_File_Chooser* chooser = new Fl_Native_File_Chooser(1); // browse directory
	chooser->show();
	if (chooser->filename() != "")
	{
		addpath(chooser->filename());
		deletedpaths.erase(chooser->filename());
	}
	updatepaths();
	std::string temp;
	std::vector<std::string> lines;
	std::ifstream config("mcpppp.properties");
	while (config)
	{
		getline(config, temp);
		lines.push_back(temp);
		if (temp.find("# GUI") == 0)
		{
			break;
		}
	}
	config.close();
	if (!lines.back().find("# GUI") == 0)
	{
		lines.push_back("# GUI (DO NOT EDIT ANY LINES AFTER THIS LINE, OR THEY MAY BE DELETED)");
	}
	lines.insert(lines.end(), paths.begin(), paths.end());
	std::ofstream fout("mcpppp.properties");
	for (std::string str : lines)
	{
		fout << str << std::endl;
	}
	for (std::string str : deletedpaths)
	{
		fout << "#!" << str << std::endl;
	}
	fout.close();
	std::cout << chooser->filename() << std::endl;
}

// callback for "Delete" button in "Edit Paths"
void deleterespath(Fl_Button* o, void* v)
{
	if (selectedwidget == nullptr)
	{
		return;
	}
	ui->paths->remove(selectedwidget);
	delete selectedwidget;
	paths.erase(selectedwidget->label());
	deletedpaths.insert(selectedwidget->label());
	updatepaths();
	ui->paths->hide();
	ui->paths->show();
}

// callback for paths buttons in "Edit Paths"
void selectpath(Fl_Radio_Button* o, void* v)
{
	selectedwidget = o;
}

// add resourcepack to checklist
void addpack(std::string name)
{
	int w, h, dx, dy;
	ui->scroll->begin();
	Fl_Check_Button* o = new Fl_Check_Button(445, 60 + 15 * numbuttons, 512, 15);
	o->copy_label(name.c_str());
	o->down_box(FL_DOWN_BOX);
	o->value(1);
	o->user_data((void*)(numbuttons));
	o->callback((Fl_Callback*)(resourcepack));
	o->when(FL_WHEN_CHANGED);
	fl_text_extents(name.c_str(), dx, dy, w, h);
	o->resize(445, 60 + 15 * numbuttons, w + 30, 15);
	ui->scroll->end();
	numbuttons++;
}

// update paths in "Edit Paths"
void updatepaths()
{
	std::string pstr;
	for (std::string str : paths)
	{
		pstr += str + " // ";
	}
	if (paths.size())
	{
		pstr.erase(pstr.end() - 4, pstr.end()); // erase the last " // "
	}
	ui->path_input->value(pstr.c_str());
}

// add paths to "Edit Paths" from paths
void addpaths()
{
	int i = 0, dx, dy, w, h;
	ui->scroll->clear();
	ui->scroll->begin();
	for (std::string str : paths)
	{
		Fl_Radio_Button* o = new Fl_Radio_Button(10, 15 + 15 * i, 250, 15);
		o->copy_label(str.c_str());
		o->callback((Fl_Callback*)(selectpath));
		fl_text_extents(str.c_str(), dx, dy, w, h);
		o->resize(10, 15 + 15 * i, std::max(w + 30, 250), 15);
		i++;
	}
	ui->scroll->end();
}

// add path to "Edit Paths" and paths
void addpath(std::string name)
{
	if (paths.find(name) != paths.end())
	{
		return;
	}
#ifdef GUI
	int w, h, dx, dy;
	ui->paths->begin();
	Fl_Radio_Button* o = new Fl_Radio_Button(10, 15 + 15 * paths.size(), 250, 15);
	o->copy_label(name.c_str());
	o->callback((Fl_Callback*)(selectpath));
	fl_text_extents(name.c_str(), dx, dy, w, h);
	o->resize(10, 15 + 15 * paths.size(), std::max(w + 30, 250), 15);
	ui->scroll->end();
#endif
	paths.insert(name);
}

#endif
