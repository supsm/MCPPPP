#include <fstream>
#include <string>

#include "json.hpp"
#include "lodepng.cpp"

#ifdef _WIN32
#include <direct.h> //zippy wants this
#endif

#include "Zippy.hpp"

extern bool autodeletetemp = false, pauseonexit = true, dolog = false, dotimestamp = false, deletesource = false;
extern int outputlevel = 3, loglevel = 2;
extern std::ofstream logfile("log.txt");

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
			if (first)
			{
				std::cout << (dotimestamp ? timestamp() : "");
			}
			std::cout << value;
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
			if (first)
			{
				std::cout << (dotimestamp ? timestamp() : "");
			}
			std::cout << str;
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
			if (first)
			{
				std::cout << (dotimestamp ? timestamp() : "");
			}
			std::cout << f;
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
