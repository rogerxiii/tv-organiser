#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>
#include <Windows.h>

#include "patterns.h"

// A few global variables
bool g_custom = false, g_intact = false, g_original = false, g_recursive = false; //, g_subtitles = false;
namespace fs = std::filesystem;

// Add to these when needed
std::unordered_set<std::wstring> extensions = { L".mp4", L".avi", L".mkv" };
std::unordered_set<std::wstring> subs_extensions = { L".srt", L".ssa", L".sub" };
std::unordered_set<wchar_t> illegal_chars = { L'\\', L'/', L':', L'*', L'?', L'"', L'<', L'>', L'|' };



int wtoi(wchar_t& wchar)
{
	switch (wchar)
	{
	case L'0': return 0;
	case L'1': return 1;
	case L'2': return 2;
	case L'3': return 3;
	case L'4': return 4;
	case L'5': return 5;
	case L'6': return 6;
	case L'7': return 7;
	case L'8': return 8;
	case L'9': return 9;
	default: return -1;
	}
}

episode_data get_episode(const std::wstring& name)
{
	episode_data ep;

	for (std::wstring& pattern : episode_patterns)
	{
		if (int pos = find_pattern(pattern, name, ep); pos)
		{
			episode_data double_ep;
			for (std::wstring& double_pattern : double_episode_patterns)
			{
				if (find_pattern(double_pattern, name, double_ep, pos))
				{
					if (ep.episode + 1 == double_ep.episode)
					{
						ep.is_double = true;
						break;
					}
				}
			}
			return ep;
		}
	}

	return episode_data();
}

bool is_video(fs::path path)
{
	std::wstring name = path.filename().wstring();
	std::wstring ext = path.extension().wstring();

	return extensions.count(ext) > 0 && name.find(L"sample") == std::string::npos;
}

bool is_subtitle(fs::path path)
{
	return subs_extensions.count(path.extension().wstring()) > 0;
}

bool has_country_code(const std::wstring& name)
{
	int l = name.length();
	return name[l - 4] == L'-' && iswalpha(name[l - 3]) && iswalpha(name[l - 2]) && iswalpha(name[l - 1]);
}

std::wstring string_to_wstring(std::string& in)
{
	int char_count = MultiByteToWideChar(CP_UTF8, 0, in.c_str(), -1, 0, 0);
	wchar_t* buf = new wchar_t[char_count];
	MultiByteToWideChar(CP_UTF8, 0, in.c_str(), -1, buf, char_count);
	std::wstring out = buf;
	delete[] buf;
	return out;
}

/* 
	Removes illegal characters from season/episode name
*/
void sanitize_name(std::wstring& name)
{
	for (int i = 0; i < name.size(); i++)
	{
		if (illegal_chars.count(name.at(i)) > 0)
		{
			if (i == 0) name = name.substr(1);
			else if (i == name.size() - 1) name = name.substr(0, i);
			else name = name.substr(0, i) + name.substr(i + 1);
		}
	}
}

std::wstring get_episode_name_from_data(std::wstring& data, int episode)
{
	std::wstring search_text = L"ep" + std::to_wstring(episode) + L"\"";
	int title_pos = data.find(search_text.c_str());
	if (title_pos == std::string::npos) return L"";
	title_pos += 12 + (episode >= 10 ? 1 : 0);
	std::wstring ep_name = data.substr(title_pos, data.find(L'"', title_pos) - title_pos);
	sanitize_name(ep_name);
	return ep_name;
}

std::wstring convert_episode_name(std::wstring& data, episode_data& ep)
{
	std::wstring ep_name = get_episode_name_from_data(data, ep.episode), ep_name2 = L"";
	
	if (ep.is_double)
	{
		ep_name2 = get_episode_name_from_data(data, ep.episode + 1);

		int pos1 = ep_name.find(L"Part"), pos2 = ep_name2.find(L"Part");
		if (pos1 != std::string::npos && pos2 != std::string::npos && !wcscmp(ep_name.substr(0, pos1).c_str(), ep_name2.substr(0, pos2).c_str()))
		{
			std::wstring name = ep_name.substr(0, pos1);
			if (name[name.length() - 1] == L' ') name.pop_back();
			return name;
		}
	}
	
	return ep_name + (ep.is_double ? L" & " + ep_name2 : L"");
}
