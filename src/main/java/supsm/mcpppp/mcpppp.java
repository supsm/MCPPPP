package supsm.mcpppp;

import net.fabricmc.api.ModInitializer;

import java.io.File;
import java.io.InputStream;
import org.apache.commons.io.FileUtils;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

class jni
{
	private static void extract(String str)
	{
		String path = System.getProperty("java.library.path");
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
		extract("mcpppp.dll");
		extract("libmcpppp.so");
		extract("libmcpppp.dylib");
		System.loadLibrary("mcpppp");
	}
	public native void run(String path, String os);
}

public class mcpppp implements ModInitializer
{
	public static final Logger LOGGER = LogManager.getLogger();

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
		String OS = System.getProperty("os.name");
		String path = System.getProperty("user.dir") + "/resourcepacks";
		String version = System.getProperty("os.version");


		if (OS.toLowerCase().contains("mac") && getmajorversion(version) < 11)
		{
			LOGGER.error("Mac OS outdated, not running MCPPPP");
			return;
		}

		new jni().run(path, OS);
		delete("mcpppp.dll");
		delete("libmcpppp.so");
		delete("libmcpppp.dylib");
	}
}
