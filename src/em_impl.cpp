/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef __EMSCRIPTEN__
#include "pch.h"
#include "utility.h"

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/threading.h>
#include <emscripten/val.h>

std::atomic_bool running = false;
std::vector<char> pack, converted_pack;

using mcpppp::output;
using mcpppp::level_t;

// conversion finished, save file, show download button, and alert user
void conversion_finished()
{
	emscripten::val::global("window").call<void>("update_download", emscripten::val::array(converted_pack));
	EM_ASM(document.getElementById("download").removeAttribute("hidden");
	alert("Conversion finished, press Download button to download"););
}

void run_in_thread() try
{
	// hide download button
	MAIN_THREAD_EM_ASM(document.getElementById("download").setAttribute("hidden", ""));

	// ideally, zipping/unzipping would be done in-memory..
	// but I don't want to (zippy doesn't support this)
	std::ofstream fout("pack.zip");
	fout.write(pack.data(), pack.size());
	fout.close();

	mcpppp::convert({ "pack.zip" });

	output<level_t::important>("Conversion Finished");

	std::ifstream fin("pack.zip");
	converted_pack = { std::istreambuf_iterator<char>(fin), {} };
	fin.close();

	std::filesystem::remove({ "pack.zip" });

	emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_V, &conversion_finished);

	running = false;
	mcpppp::checkpoint();
}
MCPPPP_CATCH_ALL()

// start conversion process
void run()
{
	if (running)
	{
		output<level_t::warning>("conversion is already running");
		return;
	}
	running = true;
	std::thread t(run_in_thread);
	t.detach();
}

// save uploaded file to vector
// @param data  string containing .zip data. std::string is expected by embind
void save_file(const std::string& data)
{
	// TODO: this causes a bit of lag when uploading large files. At least let user know that it is uploading
	// simply overwrite existing data instead of clearing
	pack.resize(data.size());
	std::copy(data.begin(), data.end(), pack.begin());
}

EMSCRIPTEN_BINDINGS(mcpppp)
{
	emscripten::function("run", &run);
	emscripten::function("save_file", &save_file);
}
#endif
