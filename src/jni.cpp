#include "convert.h"
#include "utility.h"
#include "supsm_mcpppp_jni.h"

using mcpppp::output;
using mcpppp::level_t;
using mcpppp::c8tomb;
using mcpppp::mbtoc8;

void run(std::string path, std::string os)
try
{
	output<level_t::system_info>("MCPPPP mod");
	output<level_t::system_info>("Os: {}\n", os);
	for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(mbtoc8(path))))
	{
		mcpppp::convert(entry);
	}
}
catch (const nlohmann::json::exception& e)
{
	output<level_t::error>("FATAL JSON ERROR:\n{}", e.what());
	mcpppp::printpseudotrace();
}
catch (const Zippy::ZipLogicError& e)
{
	output<level_t::error>("FATAL ZIP LOGIC ERROR:\n{}", e.what());
	mcpppp::printpseudotrace();
}
catch (const Zippy::ZipRuntimeError& e)
{
	output<level_t::error>("FATAL ZIP RUNTIME ERROR:\n{}", e.what());
	mcpppp::printpseudotrace();
}
catch (const std::filesystem::filesystem_error& e)
{
	output<level_t::error>("FATAL FILESYSTEM ERROR:\n{}", e.what());
	mcpppp::printpseudotrace();
}
catch (const std::exception& e)
{
	output<level_t::error>("FATAL ERROR:\n{}", e.what());
	mcpppp::printpseudotrace();
}
catch (...)
{
	output<level_t::error>("UNKNOWN FATAL ERROR");
	mcpppp::printpseudotrace();
}

std::string tostring(JNIEnv* env, jstring str)
{
	const char* c = env->GetStringUTFChars(str, NULL);
	std::string s = std::string(c);
	env->ReleaseStringUTFChars(str, c);
	return s;
}

JNIEXPORT void JNICALL Java_supsm_mcpppp_jni_run(JNIEnv* env, jobject obj, jstring str, jstring os)
{
	mcpppp::autodeletetemp = true, mcpppp::pauseonexit = false, mcpppp::outputlevel = level_t::info, mcpppp::dotimestamp = true, mcpppp::autoreconvert = true;
	run(tostring(env, str), tostring(env, os));
}