#define CURL_STATICLIB
#include <curl/curl.h>
#include <cwctype>
#include <fcntl.h>	//_O_U8TEXT
#include <io.h>		//_setmode
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "functions.h"


static unsigned long write_callback(void* contents, unsigned long size, unsigned long nmemb, void* userdata)
{
	((std::string*)userdata)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

bool perform_on_folder(fs::path path, CURL* curl)
{
	std::wcout << "\nRoot working directory: " << path << "\n";

	// First we take all files out of their directories 
	for (fs::directory_entry file : fs::directory_iterator(path))
	{
		if (file.is_directory())
		{
			std::wstring name = file.path().filename().wstring();
			episode_data ep = get_episode(name);
			if (ep.episode == -1 || ep.season == -1) continue;

			bool has_video = false;
			for (fs::directory_entry infile : fs::directory_iterator(file.path()))
			{
				std::wstring inname = infile.path().filename().wstring();
				if (is_video(infile.path()) || is_subtitle(infile.path()))
				{
					std::wcout << "Moving " << name << "\n";
					std::wstring newpath = file.path().parent_path() / (name + infile.path().extension().wstring());
					fs::rename(infile, newpath);
					has_video = true;
				}
			}

			if (has_video && !g_intact) fs::remove_all(file.path());
		}
	}

	static std::wstring imdb_number = L"";

	if (static bool do_once = true; do_once)
	{
		do_once = false;
		int noname_count = 0, noname_found = 0;

		std::wifstream file("imdb.txt");
		if (!file.good())
		{
			std::wcout << "Cannot find imdb.txt file with series number, please type it in.\nIMDB series number: ";
			std::wcin >> imdb_number;
			std::wcin.get();	// Empty the buffer
		}
		else std::getline(file, imdb_number);
		file.close();
	}

	if (imdb_number.at(0) != 't') imdb_number = L"tt" + imdb_number;
	if ((imdb_number.size() == 9 || imdb_number.size() == 10) && imdb_number.at(0) == 't' && imdb_number.at(1) == 't')
	{
		static std::unordered_map<int, std::wstring> imdb_data;
		std::vector<fs::path> subs;

		if (fs::exists(path / "subs"))
			for (fs::directory_entry file : fs::directory_iterator(path / "subs"))
			{
				std::wstring name = file.path().filename().wstring();
				if (is_subtitle(file.path())) subs.emplace_back(file.path());
			}

		for (fs::directory_entry file : fs::directory_iterator(path))
		{
			if (file.is_directory()) continue;

			std::wstring name = file.path().filename().wstring();
			if (is_subtitle(file.path())) subs.emplace_back(file.path()); // We do subtitles at the end, save them here

			if (episode_data ep = get_episode(name); is_video(file.path()) && ep.episode != -1 && ep.season != -1)
			{
				// Get the shows title if we haven't already gotten it before
				if (imdb_data.count(-1) == 0)
				{
					if (!g_custom)
					{
						std::wcout << "Getting the shows name... ";
						std::string data_s = "";

						// CURL only accepts chars and not wchars, convert the web page here
						std::string web = "https://www.imdb.com/title/";
						for (wchar_t& wc : imdb_number) web += (char)(wc);
						web += '/';

						curl_easy_setopt(curl, CURLOPT_URL, web.c_str());
						curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
						curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)& data_s);
						curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/108.0.0.0 Safari/537.36");

						CURLcode res = curl_easy_perform(curl);
						if (res != CURLE_OK)
						{
							std::wcout << "\nCURL error: " << curl_easy_strerror(res) << "\n\nFatal error! Stopping program\n\n";
							std::wcout << "Press any key to continue...";
							std::wcin.get();
							return false;
						}

						std::wstring data = string_to_wstring(data_s);

						// Getting the title, normal or original
						// Note: haven't found a show yet that is translated, so the original title functionality is broken atm
						int beginpos, endpos;
						//if (int pos = data.find(L"originalTitle"); g_original && pos != std::string::npos)
						//{
							//beginpos = pos + 15;
							//endpos = data.find(L"<span", beginpos);
						//}
						//else
						//{
							beginpos = data.find(L">", data.find(L">", data.find(L"hero__pageTitle")) + 1) + 1;
							endpos = data.find(L"</", beginpos);
						//}

						std::wstring showname = data.substr(beginpos, endpos - beginpos);
						sanitize_name(showname);
						imdb_data.emplace(-1, showname);
						std::wcout << "found: " << showname << "\n";
					}

					else
					{
						std::wcout << "Enter shows name: ";
						std::wstring showname;
						std::getline(std::wcin, showname);
						std::wcout << "\n";

						sanitize_name(showname);
						imdb_data.emplace(-1, showname);
					}
				}

				// Get the imdb data if we haven't already gotten it before
				if (imdb_data.count(ep.season) == 0)
				{
					std::wcout << "Getting episode names for season " << ep.season << "... ";
					std::string data_s = "";

					// CURL only accepts chars and not wchars, convert the web page here
					std::string web = "https://www.imdb.com/title/";
					for (wchar_t& wc : imdb_number) web += (char)(wc);
					web += "/episodes?season=" + std::to_string(ep.season);

					curl_easy_setopt(curl, CURLOPT_URL, web.c_str());
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)& data_s);
					curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/108.0.0.0 Safari/537.36");

					CURLcode res = curl_easy_perform(curl);
					if (res != CURLE_OK)
					{
						std::wcout << "\nCURL error: " << curl_easy_strerror(res) << "\n\nFatal error! Stopping program\n\n";
						std::wcout << "Press any key to continue...";
						std::wcin.get();
						return false;
					}

					std::wstring data = string_to_wstring(data_s);
					imdb_data.emplace(ep.season, data);

					std::wcout << "done!\n";
				}

				// Get the episode name
				std::wstring show_name = imdb_data.at(-1);
				std::wstring ep_name = convert_episode_name(imdb_data.at(ep.season), ep);

				// Format the filename
				std::wstring final_name = file.path().parent_path() / (show_name + (ep.season < 10 ? L" S0" : L" S") + std::to_wstring(ep.season));
				final_name += (ep.episode < 10 ? L"E0" : L"E") + std::to_wstring(ep.episode);
				if (ep.is_double) final_name += ((ep.episode + 1) < 10 ? L"-E0" : L"-E") + std::to_wstring(ep.episode + 1);
				final_name += L" - " + ep_name + file.path().extension().wstring();

				std::wcout << "Converted " << final_name << "\n";
				if (!fs::exists(final_name)) fs::rename(file, final_name);
			}
		}

		if (imdb_data.count(-1) == 0) return true;
		if (subs.size() > 0 && !fs::exists(path / "subs")) fs::create_directory(path / "subs");
		std::wstring show_name = imdb_data.at(-1);

		for (fs::path& path : subs)
		{
			std::wstring dest = wcscmp(path.parent_path().filename().wstring().c_str(), L"subs") == 0 ? path.parent_path() / L"" : path.parent_path() / L"subs" / L"";
			episode_data ep = get_episode(path.filename().wstring());
			std::wstring ep_name = convert_episode_name(imdb_data.at(ep.season), ep);

			dest += show_name + (ep.season < 10 ? L" S0" : L" S") + std::to_wstring(ep.season) + (ep.episode < 10 ? L"E0" : L"E") + std::to_wstring(ep.episode);
			if (ep.is_double) dest += ((ep.episode + 1) < 10 ? L"-E0" : L"-E") + std::to_wstring(ep.episode + 1);
			dest += L" - " + ep_name;
			if (has_country_code(path.stem().wstring())) dest += path.stem().wstring().substr(path.stem().wstring().size() - 4);
			dest += path.extension().wstring();

			std::wcout << "Converted " << dest << "\n";
			fs::rename(path, dest);
		}

		return true;
	}

	std::wcout << "\n\nWrong IMDB number supplied, quitting...\n\n";
	std::wcout << "Press any key to continue...";
	std::wcin.get();
	return false;
}

int main(int nargs, char** args)
{
	// Make unicode console printing work
	_setmode(_fileno(stdout), _O_U8TEXT);

	// Deal with flags
	if (nargs == 2 && (!strcmp(args[1], "-h") || !strcmp(args[1], "--help")))
	{
		std::wcout <<
			"\nPROGRAM USAGE:\n"
			"tvorg [flags]\n\n"

			"Optional flags:\n"
			" -c --custom\t\tUse a custom series title instead of the IMDB one.\n"
			" -h --help\t\tDisplays this help screen.\n"
			" -i --intact\t\tDo not remove folders and files after moving episodes out.\n"
			" -o --original\t\tUse the \"original title\" in IMDB. Also useful for localized/translated titles.\n"
			" -r --recursive\t\tExecute this program on every subfolder in this folder instead.\n\t\t\tUseful for i.e. seasons in separate folders.\n";

		return 0;
	}
	
	for (int i = 1; i < nargs; i++)
	{
		const char* arg = args[i];

		if (!strcmp(arg, "-c") || !strcmp(arg, "--custom")) g_custom = true;
		else if (!strcmp(arg, "-i") || !strcmp(arg, "--intact")) g_intact = true;
		else if (!strcmp(arg, "-o") || !strcmp(arg, "--original")) g_original = true;
		else if (!strcmp(arg, "-r") || !strcmp(arg, "--recursive")) g_recursive = true;
	}

	// Do the actual renaming
	bool result = false;
	CURL* curl = curl_easy_init();
	
	if (!g_recursive) result = perform_on_folder(fs::current_path(), curl);
	else for (fs::directory_entry file : fs::directory_iterator(fs::current_path()))
	{
		if (file.is_directory())
		{
			result = perform_on_folder(file, curl);
			if (!result) break;
		}
	}

	if (result) std::wcout << "\n\nAll done, cleaning up!";
	curl_easy_cleanup(curl);
	return 0;
}