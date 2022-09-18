package supsm.mcpppp;

import java.io.File;
import java.io.InputStream;

import org.apache.commons.io.FileUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import net.fabricmc.api.ModInitializer;

class jni
{
	// extract a file from .jar resources to path
	private static void extract(String str, String path)
	{
		File file = new File(path + "/" + str);
		InputStream stream = jni.class.getResourceAsStream("/" + str);
		try
		{
			FileUtils.copyInputStreamToFile(stream, file);
		}
		catch (java.io.IOException e)
		{
			e.printStackTrace();
		}
	}

	static
	{
		String path = System.getProperty("user.dir") + "/" + mcpppp.lib_dir;
		int libs_loaded = 0;
		for (String s : mcpppp.jni_libs)
		{
			// try to load each library until it works
			extract(s, path);
			try
			{
				System.load(path + "/" + s);
				libs_loaded++;
			}
			catch (UnsatisfiedLinkError e)
			{
				mcpppp.LOGGER.info(e.getMessage());
			}
		}
		if (libs_loaded == 0)
		{
			mcpppp.LOGGER.error("No valid MCPPPP JNI library found");
		}
	}

	public native void run(String path, String os);
}

public class mcpppp implements ModInitializer
{
	public static final Logger LOGGER = LogManager.getLogger();
	public static final String[] jni_libs = { "mcpppp.dll", "libmcpppp.so", "libmcpppp.dylib" };
	public static final String lib_dir = "mcpppp-libs";

	// delete file
	private static void delete(String str)
	{
		try
		{
			File file = new File(str);
			file.delete();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	// extract major os version from version string
	private static int getmajorversion(String version)
    {
        String nums = new String();
        for (int i = 0; i < version.length(); i++)
        {
            if (version.charAt(i) == '.')
            {
                break;
            }
            nums += version.charAt(i);
        }
        return Integer.parseInt(nums);
    }

	@Override
	public void onInitialize()
	{
		// This code runs as soon as Minecraft is in a mod-load-ready state.
		// However, some things (like resources) may still be uninitialized.
		// Proceed with mild caution.

		// name of os
		String OS = System.getProperty("os.name");
		// minecraft directory
		String path = System.getProperty("user.dir");
		// version of os
		String version = System.getProperty("os.version");

		// jni binaries only work on MacOS 12+
		if (OS.toLowerCase().contains("mac") && getmajorversion(version) < 11)
		{
			LOGGER.error("Mac OS outdated, not running MCPPPP");
			return;
		}

		try
		{
			new jni().run(path + "/resourcepacks", OS + " " + version);
		}
		catch (UnsatisfiedLinkError e)
		{
			LOGGER.error("MCPPPP JNI library improperly loaded, conversion skipped");
		}

		for (String s : jni_libs)
		{
			delete(path + "/" + lib_dir + "/" + s);
		}
		delete(path + "/" + lib_dir);
	}
}
