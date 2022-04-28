/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef GUI
#include "gui.h"

#include <future>
#include <set>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <ShlObj.h>
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
#include <FL/fl_ask.H>

#include "utility.h"

extern void resourcepack(Fl_Check_Button*, void*);
extern void selectpath(Fl_Radio_Button*, void*) noexcept;
extern void savesettings(Fl_Button* o, void* v);

using mcpppp::c8tomb;
using mcpppp::mbtoc8;

namespace mcpppp
{
	// get default resourcepacks path to use based on system
	static std::u8string getdefaultpath()
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
		return mbtoc8(str);
#elif defined __linux__
		return u8"~/.minecraft/resourcepacks";
#elif defined __APPLE__ || defined __MACH__
		return u8"~/Library/Application Support/minecraft/resourcepacks";
#else
		return nullptr;
#endif
	}

	void guirun()
		try
	{
		output<level_t::important>("Conversion Started");
		bool valid = false;
		for (const std::pair<bool, std::filesystem::directory_entry>& p : entries)
		{
			if (!p.first)
			{
				continue;
			}
			if (convert(p.second, dofsb, dovmt, docim))
			{
				valid = true;
			}
			else
			{
				if (p.second.is_directory() || p.second.path().extension() == ".zip")
				{
					valid = true;
				}
			}
		}
		if (!valid)
		{
			output<level_t::warning>("No valid path found, running from default directory: {}", c8tomb(getdefaultpath()));
			for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(getdefaultpath())))
			{
				convert(std::filesystem::canonical(entry), dofsb, dovmt, docim);
			}
		}
		running = false;
		output<level_t::important>("Conversion Finished");
	}
	catch (const nlohmann::json::exception& e)
	{
		output<level_t::error>("FATAL JSON ERROR:\n{}", e.what());
		mcpppp::printpseudotrace();
		std::exit(-1);
	}
	catch (const Zippy::ZipLogicError& e)
	{
		output<level_t::error>("FATAL ZIP LOGIC ERROR:\n{}", e.what());
		mcpppp::printpseudotrace();
		std::exit(-1);
	}
	catch (const Zippy::ZipRuntimeError& e)
	{
		output<level_t::error>("FATAL ZIP RUNTIME ERROR:\n{}", e.what());
		mcpppp::printpseudotrace();
		std::exit(-1);
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		output<level_t::error>("FATAL FILESYSTEM ERROR:\n{}", e.what());
		mcpppp::printpseudotrace();
		std::exit(-1);
	}
	catch (const std::exception& e)
	{
		output<level_t::error>("FATAL ERROR:\n{}", e.what());
		mcpppp::printpseudotrace();
		std::exit(-1);
	}
	catch (...)
	{
		output<level_t::error>("UNKNOWN FATAL ERROR");
		mcpppp::printpseudotrace();
		std::exit(-1);
	}

#ifdef _WIN32
	// get scale of window (windows)
	static UINT win_getscale() noexcept
	{
		SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
		return GetDpiForSystem();
	}
#endif

	// get scale for text
	static double getscale()
	{
#ifdef _WIN32
		std::future<UINT> dpi = std::async(&win_getscale);
		return dpi.get() / 96.0;
#else
		return 1;
#endif
	}

	void addpack(const std::filesystem::path& path, const bool selected)
	{
		using mcpppp::checkresults;
		const auto statuses = mcpppp::getconvstatus(path, mcpppp::dofsb, mcpppp::dovmt, mcpppp::docim);
		checkresults status = checkresults::noneconvertible;
		for (const auto& [key, value] : statuses)
		{
			switch (value)
			{
			case checkresults::valid:
				status = value;
				break;
			case checkresults::reconverting:
				if (status != checkresults::valid)
				{
					status = value;
				}
				break;
			case checkresults::alrfound:
				if (status == checkresults::noneconvertible)
				{
					status = value;
				}
				break;
			case checkresults::noneconvertible:
				break;
			}
		}

		std::array<Fl_Color, 4> result_colors = { FL_DARK_GREEN, FL_DARK3, FL_RED, 93 };
		std::array<std::string, 4> result_names = { "Convertible", "No files found to convert", "Converted files already found", "Possibly will reconvert" };

		// only w is used
		int w = 0, h = 0, dx = 0, dy = 0;
		fl_text_extents(c8tomb(path.filename().u8string().c_str()), dx, dy, w, h);
		w = std::lround(w / getscale());
		std::unique_ptr<Fl_Check_Button> o = std::make_unique<Fl_Check_Button>(445, 60 + 15 * numbuttons, w + 30, 15);
		std::unique_ptr<int> temp = std::make_unique<int>(numbuttons);
		o->labelcolor(result_colors[static_cast<size_t>(status)]);
		o->copy_label(c8tomb(path.filename().u8string().c_str()));
		o->copy_tooltip((result_names[static_cast<size_t>(status)] + '\n' + c8tomb(path.generic_u8string())).c_str());
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

	void updatepaths()
	{
		std::u8string pstr;
		for (const std::filesystem::path& str : paths)
		{
			pstr.append(str.generic_u8string() + u8" // ");
		}
		if (!paths.empty())
		{
			pstr.erase(pstr.end() - 4, pstr.end()); // erase the last " // "
		}
		ui->path_input->value(c8tomb(pstr.c_str()));
	}

	void updatesettings()
	{
		for (const auto& [key, value] : settings)
		{
			switch (value.type)
			{
			case type_t::boolean:
				std::get<0>(settings_widgets[key]).first->value(static_cast<int>(value.get<bool>()));
				std::get<0>(settings_widgets[key]).second->value(static_cast<int>(!value.get<bool>()));
				break;
			case type_t::integer:
				std::get<1>(settings_widgets[key])->value(static_cast<int>(value.get<level_t>()));
				break;
			case type_t::string:
				std::get<2>(settings_widgets[key])->value(value.get<std::string>().c_str());
				break;
			}
		}
	}

	void updatepathconfig()
	{
		auto temppaths = paths;

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
			output<level_t::error>("Error while parsing config: {}", e.what());
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
							deletedpaths.erase(mbtoc8(path)); // we don't need to delete paths which are in gui (we can override)
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
					temppaths.erase(mbtoc8(path)); // we only need to add paths which haven't already been added (outside of gui)
				}
			}
		}

		std::vector<std::string> tempv;
		std::transform(temppaths.begin(), temppaths.end(), std::back_inserter(tempv), [](const std::filesystem::path& path) -> std::string
			{
				return c8tomb(path.generic_u8string());
			});
		config["gui"]["paths"] = tempv;
		tempv.clear();
		std::transform(deletedpaths.begin(), deletedpaths.end(), std::back_inserter(tempv), [](const std::filesystem::path& path) -> std::string
			{
				return c8tomb(path.generic_u8string());
			});
		config["gui"]["excludepaths"] = tempv;

		std::ofstream fout("mcpppp-config.json");
		fout << "// Please check out the Documentation for the config file before editing it yourself: https://github.com/supsm/MCPPPP/blob/master/CONFIG.md" << std::endl;
		fout << config.dump(1, '\t') << std::endl;
		fout.close();
	}

	void addpaths()
	{
		// only w, maxsize, and i are used
		int i = 0, dx = 0, dy = 0, w = 0, h = 0, maxsize = 0;
		ui->paths->clear();
		// make sure all boxes are same size
		for (const auto& path : paths)
		{
			fl_text_extents(c8tomb((u8"    " + path.filename().u8string()).c_str()), dx, dy, w, h);
			w = std::lround(w / getscale());
			maxsize = std::max(maxsize, w);
		}
		for (const auto& path : paths)
		{
			std::unique_ptr<Fl_Radio_Button> o = std::make_unique<Fl_Radio_Button>(10, 15 + 15 * i, std::max(maxsize, 250), 15);
			o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
			o->copy_label(c8tomb((u8"    " + path.generic_u8string()).c_str()));
			o->callback(reinterpret_cast<Fl_Callback*>(selectpath));
			ui->paths->add(o.get());
			o.release();

			i++;
		}
	}

	void addpath(const std::filesystem::path& path)
	{
		const std::filesystem::path canonical = std::filesystem::canonical(path);
		if (paths.contains(canonical))
		{
			return;
		}

		// if name does not end in .minecraft/resourcepacks
		const size_t find = canonical.generic_u8string().rfind(u8".minecraft/resourcepacks");
		if (!ui->dontshowwarning->value() && (find < canonical.generic_u8string().size() - 24 || find == std::string::npos))
		{
			// don't let user do anything until they close window lol
			ui->path_warning->set_modal();
			ui->path_warning->show();
			while (ui->path_warning->shown())
			{
				Fl::wait();
			}
		}

		paths.insert(canonical);
		addpaths();
	}

	void init_settings()
	{

		ui->settings = new Fl_Double_Window(300, 95 + (settings.size() - 1) * 30, "Settings");

		int curnum = 0;
		for (const auto& [key, value] : settings)
		{
			// setting name and description as tooltip
			{
				Fl_Box* setting_name = new Fl_Box(10, 10 + curnum * 30, 120, 20, value.formatted_name.data());
				setting_name->tooltip(value.description.data());
				setting_name->align(Fl_Align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE));
			}

			// widget(s) to update setting value
			switch (value.type)
			{
				// true/false buttons for boolean settings
			case type_t::boolean:
			{
				Fl_Group* button_group = new Fl_Group(140, 10 + curnum * 30, 150, 20);

				Fl_Button* truebutton = new Fl_Button(140, 10 + curnum * 30, 75, 20, "True");
				truebutton->type(FL_RADIO_BUTTON);
				truebutton->box(FL_FLAT_BOX);
				truebutton->down_box(FL_BORDER_BOX);
				truebutton->value(value.get<bool>());
				truebutton->color(FL_DARK2);
				truebutton->selection_color(43);
				truebutton->callback(reinterpret_cast<Fl_Callback*>(settingchanged));

				Fl_Button* falsebutton = new Fl_Button(215, 10 + curnum * 30, 75, 20, "False");
				falsebutton->type(FL_RADIO_BUTTON);
				falsebutton->box(FL_FLAT_BOX);
				falsebutton->down_box(FL_BORDER_BOX);
				falsebutton->value(!value.get<bool>());
				falsebutton->color(FL_DARK2);
				falsebutton->selection_color(43);
				falsebutton->callback(reinterpret_cast<Fl_Callback*>(settingchanged));

				settings_widgets[key] = std::make_pair(truebutton, falsebutton);
				button_group->end();
				break;
			}

			// counter for integer settings
			case type_t::integer:
			{
				Fl_Counter* counter = new Fl_Counter(140, 10 + curnum * 30, 150, 20);
				counter->box(FL_BORDER_BOX);
				counter->labeltype(FL_NO_LABEL);
				counter->minimum(value.min);
				counter->maximum(value.max);
				counter->step(1);
				counter->value(static_cast<int>(value.get<level_t>()));
				counter->callback(reinterpret_cast<Fl_Callback*>(settingchanged));

				settings_widgets[key] = counter;
				break;
			}

			// text input for string settings
			case type_t::string:
			{
				Fl_Input* input = new Fl_Input(140, 10 + curnum * 30, 150, 20);
				input->box(FL_BORDER_BOX);
				input->labeltype(FL_NO_LABEL);
				input->value(value.get<std::string>().c_str());
				input->callback(reinterpret_cast<Fl_Callback*>(settingchanged));
				input->when(FL_WHEN_CHANGED);

				settings_widgets[key] = input;
				break;
			}
			}

			curnum++;
		}

		{
			Fl_Button* savebutton = new Fl_Button(10, 20 + curnum * 30, 280, 25, "Save");
			savebutton->tooltip("Save settings");
			savebutton->box(FL_BORDER_BOX);
			savebutton->down_box(FL_BORDER_BOX);
			savebutton->color(FL_DARK2);
			savebutton->selection_color(43);
			savebutton->callback(reinterpret_cast<Fl_Callback*>(savesettings));
		}
		{
			savewarning = new Fl_Box(10, 45 + curnum * 30, 280, 20, "Warning: unsaved changes");
			savewarning->labelfont(1);
			savewarning->labelcolor((Fl_Color)1);
			savewarning->hide();
		}
		// TODO: add save and savewarning
	}

#ifdef _WIN32
	std::string winfilebrowser()
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
