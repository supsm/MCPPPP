/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef GUI
#include <thread>
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Radio_Button.H>
#include <FL/Fl_Native_File_Chooser.H>
#include "FL/fl_ask.H"

#include "gui.h"
#include "utility.h"

static std::unique_ptr<Fl_Widget> selectedwidget;

using mcpppp::ui;
using mcpppp::out;
using mcpppp::paths;
using mcpppp::entries;
using mcpppp::deletedpaths;
using mcpppp::c8tomb;
using mcpppp::mbtoc8;

// callback for run button
void run(Fl_Button* o, void* v)
{
	if (mcpppp::running)
	{
		return;
	}
	mcpppp::running = true;
	std::thread t(mcpppp::guirun);
	t.detach();
}

// callback for "CIM", "VMT", "FSB" checkboxes
void conversion(Fl_Check_Button* o, void* v)
{
	if (strcmp(o->label(), "FSB") == 0)
	{
		mcpppp::dofsb = static_cast<bool>(o->value());
	}
	if (strcmp(o->label(), "VMT") == 0)
	{
		mcpppp::dovmt = static_cast<bool>(o->value());
	}
	if (strcmp(o->label(), "CIM") == 0)
	{
		mcpppp::docim = static_cast<bool>(o->value());
	}
}

// callback for resourcepack checkboxes
void resourcepack(Fl_Check_Button* o, void* v)
{
	entries.at(static_cast<size_t>(*(static_cast<int*>(v)))).first = static_cast<bool>(o->value());
	ui->allpacks->value(0);
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
	mcpppp::numbuttons = 0;
	ui->scroll->begin(); // padding
	std::unique_ptr<Fl_Check_Button> pad = std::make_unique<Fl_Check_Button>(470, 45, 150, 15);
	pad->down_box(FL_DOWN_BOX);
	pad->labeltype(FL_NO_LABEL);
	pad->hide();
	pad->deactivate();
	ui->scroll->end();
	for (const std::filesystem::path& path : paths)
	{
		if (std::filesystem::is_directory(path))
		{
			for (const auto& entry : std::filesystem::directory_iterator(path))
			{
				if (entry.is_directory() || entry.path().extension() == ".zip")
				{
					entries.push_back(std::make_pair(true, entry));
					mcpppp::addpack(entry.path(), true);
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
	// std::string::contains in C++23
	while (str.find(" // ") != std::string::npos)
	{
		const size_t i = str.find(" // ");
		const std::u8string temppath = std::u8string(str.begin(), str.begin() + static_cast<std::string::difference_type>(i));
		const std::filesystem::path path = std::filesystem::canonical(temppath);
		paths.insert(path);
		deletedpaths.erase(path);
		str.erase(str.begin(), str.begin() + static_cast<std::string::difference_type>(i + 4));
	}
	if (!str.empty())
	{
		const std::filesystem::path path = std::filesystem::canonical(mbtoc8(str));
		paths.insert(path);
		deletedpaths.erase(path);
	}
	mcpppp::addpaths();
	mcpppp::updatepathconfig();
}

// callback for "Add" button in "Edit Paths"
void addrespath(Fl_Button* o, void* v)
{
	std::filesystem::path path;
#ifdef _WIN32
	path = mbtoc8(mcpppp::winfilebrowser());
#else
	std::unique_ptr<Fl_Native_File_Chooser> chooser = std::make_unique<Fl_Native_File_Chooser>(Fl_Native_File_Chooser::BROWSE_DIRECTORY); // browse directory
	chooser->show();
	path = mbtoc8(chooser->filename());
#endif
	path = std::filesystem::canonical(path);
	if (!path.empty() && !paths.contains(path))
	{
		mcpppp::addpath(path);
		deletedpaths.erase(path);
		mcpppp::updatepaths();
		mcpppp::updatepathconfig();
		ui->edit_paths->redraw();
		reload(nullptr, nullptr);
	}
}

// callback for "Delete" button in "Edit Paths"
void deleterespath(Fl_Button* o, void* v)
{
	if (selectedwidget == nullptr)
	{
		return;
	}
	std::u8string s = mbtoc8(selectedwidget->label());
	// erase spaces used for padding
	s.erase(s.begin(), s.begin() + 4);
	std::filesystem::path path = std::filesystem::canonical(s);

	paths.erase(path);
	deletedpaths.insert(path);
	selectedwidget.reset();
	mcpppp::addpaths();
	mcpppp::updatepaths();
	ui->paths->hide();
	ui->paths->show();
	mcpppp::updatepathconfig();
	ui->edit_paths->redraw();
	reload(nullptr, nullptr);
}

// callback for paths buttons in "Edit Paths"
void selectpath(Fl_Radio_Button* o, void* v) noexcept
{
	selectedwidget.reset(o);
}

// callback for settings button
void opensettings(Fl_Button* o, void* v)
{
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
	using mcpppp::config;

	// TODO: automate addition of settings here?
	// find some way to store pointer to widgets/value() functions in gui but not cli
	config["gui"]["settings"]["autoDeleteTemp"] = static_cast<bool>(ui->autodeletetemptrue->value());
	config["gui"]["settings"]["log"] = ui->log->value();
	config["gui"]["settings"]["timestamp"] = static_cast<bool>(ui->timestamptrue->value());
	config["gui"]["settings"]["logLevel"] = ui->loglevel->value();
	config["gui"]["settings"]["autoReconvert"] = static_cast<bool>(ui->autoreconverttrue->value());
	config["gui"]["settings"]["fsbTransparent"] = static_cast<bool>(ui->fsbtransparenttrue->value());

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
	// remove duplicate settings (settings that are the same in gui and non-gui config)
	if (j.contains("settings"))
	{
		if (j["settings"].type() == nlohmann::json::value_t::object)
		{
			for (const auto& setting : j["settings"].items())
			{
				for (const auto& setting2 : config["gui"]["settings"].items())
				{
					if (mcpppp::lowercase(setting.key()) == mcpppp::lowercase(setting2.key()))
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
	// remove default settings
	for (const auto& setting : config["gui"]["settings"].items())
	{
		if (mcpppp::settings.at(mcpppp::lowercase(setting.key())).default_val == nlohmann::json(setting.value()))
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

	mcpppp::readconfig();

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
	mcpppp::numbuttons = 0;
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
		mcpppp::addpack(entry.second.path(), static_cast<bool>(o->value()));
		entry.first = static_cast<bool>(o->value());
	}
	ui->scroll->redraw();
	Fl::wait();
}

// callback for output level slider
void updateoutputlevel(Fl_Value_Slider* o, void* v)
{
	mcpppp::waitdontoutput = true;
	mcpppp::outputlevel = ui->outputlevelslider->value();
	ui->output->clear();
	mcpppp::output_mutex.lock();
	for (const auto& p : mcpppp::outputted)
	{
		if (p.first >= mcpppp::outputlevel)
		{
			ui->output->add(("@S14@C" + std::to_string(mcpppp::outstream::colors.at(p.first - 1)) + "@." + p.second).c_str());
		}
	}
	mcpppp::output_mutex.unlock();
	ui->output->bottomline(ui->output->size());
	mcpppp::waitdontoutput = false;
	// save outputlevel setting
	mcpppp::config["gui"]["settings"]["outputLevel"] = mcpppp::outputlevel;
	std::ofstream fout("mcpppp-config.json");
	fout << "// Please check out the Documentation for the config file before editing it yourself: https://github.com/supsm/MCPPPP/blob/master/CONFIG.md" << std::endl;
	fout << mcpppp::config.dump(1, '\t') << std::endl;
	fout.close();
}

// callback for closing main window
void windowclosed(Fl_Double_Window* o, void* v)
{
	if (mcpppp::running)
	{
		switch (fl_choice("Conversion is still in progress, files may be corrupted if you choose to close now. Are you sure you want to close?",
			"Close", "Don't Close", nullptr))
		{
		case 0: // if yes, close window
			o->hide();
			break;
		case 1: // don't do anything
			break;
		default:
			break;
		}
	}
	else
	{
		o->hide();
	}
}
#endif
