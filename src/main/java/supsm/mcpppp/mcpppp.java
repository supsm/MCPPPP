package supsm.mcpppp;

import com.google.gson.*;
import net.fabricmc.api.ModInitializer;

import java.io.File;
import java.io.InputStream;
import org.apache.commons.io.FileUtils;

import java.util.Locale;
import java.util.concurrent.TimeUnit;

public class mcpppp implements ModInitializer {
	private void extract(String str)
	{
		File file = new File(str);
		InputStream stream = getClass().getResourceAsStream("/" + str);
		try
		{
			FileUtils.copyInputStreamToFile(stream, file);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}
	private void delete(String str)
	{
		File file = new File(str);
		file.delete();
	}
	@Override
	public void onInitialize() {
		// This code runs as soon as Minecraft is in a mod-load-ready state.
		// However, some things (like resources) may still be uninitialized.
		// Proceed with mild caution.

		String OS = System.getProperty("os.name").toLowerCase();
		String path = System.getProperty("user.dir") + "/resourcepacks";
		JsonObject arguments = new JsonParser().parse("{\"settings\":{\"pauseonexit\":false,\"timestamp\":true}}").getAsJsonObject();
		JsonArray array = new JsonArray();
		array.add(path);
		arguments.add("paths", array);

		String s = arguments.toString();
		s = s.replaceAll("\"", "\\\\\"");

		if (OS.contains("windows"))
		{
			extract("MCPPPP-windows-cli.exe");
			try
			{
				ProcessBuilder pb = new ProcessBuilder("MCPPPP-windows-cli", s);
				pb.inheritIO();
				Process p = pb.start();
				while (p.isAlive())
				{
					TimeUnit.SECONDS.sleep(1);
				}
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
			delete("MCPPPP-windows-cli.exe");
		}
		else if (OS.contains("linux"))
		{
			extract("MCPPPP-linux");
			try
			{
				ProcessBuilder pb = new ProcessBuilder("./MCPPPP-linux", s);
				pb.inheritIO();
				Process p = pb.start();
				while (p.isAlive())
				{
					TimeUnit.SECONDS.sleep(1);
				}
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
			delete("MCPPPP-linux");
		}
		else if (OS.contains("mac"))
		{
			extract("MCPPPP-mac-cli.exe");
			try
			{
				ProcessBuilder pb = new ProcessBuilder("./MCPPPP-mac-cli", s);
				pb.inheritIO();
				Process p = pb.start();
				while (p.isAlive())
				{
					TimeUnit.SECONDS.sleep(1);
				}
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
			delete("MCPPPP-mac-cli.exe");
		}
		else
		{
			System.out.println("Unrecognized OS, skipping MCPPPP conversion");
		}
	}
}
