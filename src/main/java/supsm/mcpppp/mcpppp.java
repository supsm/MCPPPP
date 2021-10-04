package supsm.mcpppp;

import net.fabricmc.api.ModInitializer;

import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import org.apache.commons.io.FileUtils;

import java.util.Locale;
import java.util.concurrent.TimeUnit;

class jni
{
	private static void extract(String str) {
		File file = new File(str);
		InputStream stream = jni.class.getResourceAsStream("/" + str);
		try {
			FileUtils.copyInputStreamToFile(stream, file);
		} catch (java.io.IOException e) {
			e.printStackTrace();
		}
	}
	static {
		extract("mcpppp.dll");
		extract("libmcpppp.so");
		extract("libmcpppp.dylib");
		System.loadLibrary("mcpppp");
	}
	public native void run(String path, String os);
}

public class mcpppp implements ModInitializer {
	private void delete(String str)
	{
		try {
			File file = new File(str);
			file.delete();
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}
	@Override
	public void onInitialize() {
		// This code runs as soon as Minecraft is in a mod-load-ready state.
		// However, some things (like resources) may still be uninitialized.
		// Proceed with mild caution.
		String OS = System.getProperty("os.name").toLowerCase();
		String path = System.getProperty("user.dir") + "/resourcepacks";

		new jni().run(path, OS);
		delete("mcpppp.dll");
		delete("libmcpppp.so");
		delete("libmcpppp.dylib");
	}
}
