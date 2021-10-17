#include "fsb.cpp"
#include "vmt.cpp"
#include "cim.cpp"
#include "supsm_mcpppp_jni.h"


void run(std::string path, std::string os) try
{
	out(5) << "MCPPPP mod" << std::endl;
	out(5) << "Os: " << os << std::endl << std::endl;
	for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(path)))
	{
		if (entry.is_directory())
		{
			fsb(entry.path().u8string(), entry.path().filename().u8string(), false);
			vmt(entry.path().u8string(), entry.path().filename().u8string(), false);
			cim(entry.path().u8string(), entry.path().filename().u8string(), false);
		}
		else if (entry.path().extension() == ".zip")
		{
			fsb(entry.path().u8string(), entry.path().filename().u8string(), true);
			vmt(entry.path().u8string(), entry.path().filename().u8string(), true);
			cim(entry.path().u8string(), entry.path().filename().u8string(), true);
		}
	}
}
catch (const nlohmann::json::exception& e)
{
	out(5) << "FATAL JSON ERROR:" << std::endl << e.what() << std::endl;
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
	autodeletetemp = true, pauseonexit = false, outputlevel = 2, dotimestamp = true;
	run(tostring(env, str), tostring(env, os));
}