/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef GUI
#include <thread>
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Radio_Button.H>
#include "FL/fl_ask.H"

#include "gui.h"
#include "utility.h"

static std::unique_ptr<Fl_Widget> selectedwidget;

using mcpppp::ui;
using mcpppp::output;
using mcpppp::level_t;
using mcpppp::paths;
using mcpppp::entries;
using mcpppp::deletedpaths;
using mcpppp::c8tomb;
using mcpppp::mbtoc8;
using mcpppp::checkpoint_only;

// callback for run button
void run(Fl_Button* o, void* v)
{
	if (mcpppp::running)
	{
		// conversion is already paused, unpause it
		if (mcpppp::pause_conversion)
		{
			o->label("Pause");
			o->tooltip("Pause conversion");
		}
		// pause conversion process
		else
		{
			o->label("Resume");
			o->tooltip("Resume conversion");
		}
		mcpppp::pause_conversion = !mcpppp::pause_conversion;
		return;
	}
	mcpppp::running = true;
	mcpppp::pause_conversion = false;
	o->label("Pause");
	o->tooltip("Pause conversion");
	std::thread t(mcpppp::guirun);
	t.detach();
	checkpoint_only();
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
	checkpoint_only();
}

// callback for resourcepack checkboxes
void resourcepack(Fl_Check_Button* o, void* v)
{
	entries.at(static_cast<size_t>(*(static_cast<int*>(v)))).selected = static_cast<bool>(o->value());
	ui->allpacks->value(0);
	checkpoint_only();
}

// callback for Force Reconvert checkbox (accessed by right clicking resourcepack name)
void forcereconvert(Fl_Menu_* o, void* v)
{
	const auto menu_item = o->mvalue();
	auto& entry = entries.at(static_cast<size_t>(*(static_cast<int*>(v))));
	const auto result = mcpppp::combine_checkresults(entry.conv_statuses);

	if (result != mcpppp::checkresults::reconverting)
	{
		return;
	}

	entry.force_reconvert = static_cast<bool>(menu_item->value());
	entry.label_widget->copy_tooltip(mcpppp::get_pack_tooltip_name(
		result,
		static_cast<bool>(menu_item->value()),
		entry.path_entry.path()).c_str());
	if (menu_item->value() == 0)
	{
		entry.label_widget->labelcolor(mcpppp::result_colors.at(static_cast<size_t>(result)));
	}
	else
	{
		entry.label_widget->labelcolor(std::get<static_cast<size_t>(mcpppp::checkresults::valid)>(mcpppp::result_colors));
	}

	ui->scroll->redraw();
	checkpoint_only();
}

// callback for browse button
void browse(Fl_Button* o, void* v)
{
	ui->edit_paths->show();
	checkpoint_only();
}

// callback for reload button
void reload(Fl_Button* o, void* v)
{
	if (mcpppp::running)
	{
		return;
	}
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
					mcpppp::addpack(entry, true);
				}
			}
		}
	}
	ui->allpacks->value(1);
	Fl::wait();
	checkpoint_only();
}

// callback for path_input
void editpath(Fl_Input* o, void* v)
{
	std::string str = o->value();

	// add all current paths to deleted paths, clear current paths
	std::set<std::filesystem::path> new_paths;
	std::set<std::filesystem::path> new_deletedpaths = deletedpaths;
	new_deletedpaths.insert(paths.begin(), paths.end());

	// for each path entry, add to paths and remove from deleted paths
	try
	{
		// std::string::contains in C++23
		while (str.find(" // ") != std::string::npos)
		{
			const size_t i = str.find(" // ");
			const std::u8string temppath = std::u8string(str.begin(), str.begin() + static_cast<std::string::difference_type>(i));
			const std::filesystem::path path = std::filesystem::canonical(temppath);
			new_paths.insert(path);
			new_deletedpaths.erase(path);
			str.erase(str.begin(), str.begin() + static_cast<std::string::difference_type>(i + 4));
		}
		// final path entry is not succeeded by " // "
		if (!str.empty())
		{
			const std::filesystem::path path = std::filesystem::canonical(mbtoc8(str));
			new_paths.insert(path);
			new_deletedpaths.erase(path);
		}

		paths = new_paths;
		deletedpaths = new_deletedpaths;

		mcpppp::addpaths();
		mcpppp::updatepathconfig();
		checkpoint_only();
	}
	// if a filesystem error occurs (most likely invalid path), changes will be discarded
	catch (const std::filesystem::filesystem_error& e)
	{
		output<level_t::error>("Filesystem error: {}\nNo changes have been made to paths", e.what());
		checkpoint_only();
	}

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
	checkpoint_only();
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
	checkpoint_only();
}

// callback for paths buttons in "Edit Paths"
void selectpath(Fl_Radio_Button* o, void* v) noexcept
{
	selectedwidget.reset(o);
	checkpoint_only();
}

// callback for settings button
void opensettings(Fl_Button* o, void* v)
{
	ui->settings->show();
	ui->box1->redraw(); // the outlining boxes disappear for some reason
	ui->box2->redraw();
	checkpoint_only();
}

// callback for help button
void openhelp(Fl_Button* o, void* v)
{
	ui->help->show();
	ui->box1->redraw();
	ui->box2->redraw();
	checkpoint_only();
}

// callback for save button in setings
void savesettings(Fl_Button* o, void* v)
{
	using mcpppp::config;
	using mcpppp::settings_widgets;

	// find some way to store pointer to widgets/value() functions in gui but not cli
	for (const auto& [key, value] : mcpppp::settings)
	{
		switch (value.type)
		{
		case mcpppp::type_t::boolean:
			config["gui"]["settings"][value.formatted_name.data()] = static_cast<bool>(std::get<0>(settings_widgets[key]).first->value());
			break;
		case mcpppp::type_t::integer:
			config["gui"]["settings"][value.formatted_name.data()] = std::get<1>(settings_widgets[key])->value();
			break;
		case mcpppp::type_t::string:
			config["gui"]["settings"][value.formatted_name.data()] = std::get<2>(settings_widgets[key])->value();
			break;
		}
	}
	checkpoint_only(); // finish updating `mcpppp::config`

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
		output<level_t::error>("Error while parsing config: {}", e.what());
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
	checkpoint_only(); // finish removing duplicate settings

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
	checkpoint_only(); // finish removing default settings

	std::ofstream fout("mcpppp-config.json");
	fout << "// Please check out the Documentation for the config file before editing it yourself: https://github.com/supsm/MCPPPP/blob/master/CONFIG.md" << std::endl;
	fout << config.dump(1, '\t') << std::endl;
	fout.close();

	mcpppp::readconfig();

	mcpppp::savewarning->hide();
	checkpoint_only();
}

// callback for edited settings
void settingchanged(Fl_Widget* o, void* v)
{
	mcpppp::savewarning->show();
	checkpoint_only();
}

// callback for select all/none
void selectall(Fl_Check_Button* o, void* v)
{
	for (auto& entry : entries)
	{
		entry.selected = static_cast<bool>(o->value());
		entry.checkbox_widget->value(o->value());
	}
	ui->scroll->redraw();
	Fl::wait();
	checkpoint_only();
}

// callback for output level slider
void updateoutputlevel(Fl_Value_Slider* o, void* v)
{
	mcpppp::waitdontoutput = true;
	mcpppp::outputlevel = static_cast<level_t>(ui->outputlevelslider->value());
	ui->output->clear();
	mcpppp::output_mutex.lock();
	for (const auto& p : mcpppp::outputted)
	{
		if (p.first >= static_cast<int>(mcpppp::outputlevel))
		{
			ui->output->add(("@S14@C" + std::to_string(mcpppp::outstream::colors.at(p.first)) + "@." + p.second).c_str());
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
	checkpoint_only();
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
	checkpoint_only();
}
#endif
