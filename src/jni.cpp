#include "convert.h"
#include "utility.h"
#include "supsm_mcpppp_jni.h"

using mcpppp::out;

void run(std::string path, std::string os)
try
{
	out(6) << "MCPPPP mod" << std::endl;
	out(6) << "Os: " << os << std::endl << std::endl;
	for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(path)))
	{
		mcpppp::convert(entry);
	}
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

std::string tostring(JNIEnv* env, jstring str)
{
	const char* c = env->GetStringUTFChars(str, NULL);
	std::string s = std::string(c);
	env->ReleaseStringUTFChars(str, c);
	return s;
}

JNIEXPORT void JNICALL Java_supsm_mcpppp_jni_run(JNIEnv* env, jobject obj, jstring str, jstring os)
{
	mcpppp::autodeletetemp = true, mcpppp::pauseonexit = false, mcpppp::outputlevel = 2, mcpppp::dotimestamp = true;
	run(tostring(env, str), tostring(env, os));
}