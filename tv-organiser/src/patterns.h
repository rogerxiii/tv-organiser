#pragma once

/*

	How to add more patterns:
	 - Add your (case-insensitive) single episode pattern/format to episode_patterns
	 - Add your (case-insensitive) double episode appended pattern/format to double_episode_patterns
		NB: This means that you only need to add the pattern of the part that's added to the single episode pattern

	Symbol definitions:
	 ?*		Variable length season number (at least 1)
	 ?#		Variable length episode number (at least 1)
	 ?x*	Specific length season number
	 ?x#	Specific length episode number

*/

std::wstring episode_patterns[] = {
	L"s?*e?#",		// Example: S01E01
	L"?*x?#",		// Example: 1x01
	L"se?* ep?#",	// Example: SE1 EP001
	L"s?* ep?#"		// Example: S1 EP001
};

std::wstring double_episode_patterns[] = {
	L"-e?#",		// Example: S01E01-E02
	L"e?#",			// Example: S01E01E02
	L" e?#",		// Example: S01E01 E02
	L"-?#"			// Example: S01E01-02
};


// ----------------------------------------------------------------------------------------------------------------------------


struct episode_data
{
	int season = -1;
	int episode = -1;
	bool is_double = false;
};

int find_pattern(std::wstring& pattern, const std::wstring& name, episode_data& ep, int start = -1)
{
	bool first = false;
	bool correct = true;
	int current = 0;
	int amount = -1;	// -1 = variable
	int style = 0;		// 0 = none, 1 = season, 2 = episode

	for (int i = 0; i < pattern.length(); ++i)
	{
		if (pattern[i] == '?')
		{
			if (i == 0) first = true;
			if (iswdigit(pattern[i + 1]))
			{
				int digit_amount = 0;
				for (int j = i + 1; j < pattern.length(); ++j)
				{
					if (iswdigit(pattern[j])) digit_amount++;
					else break;
				}
				amount = _wtoi(pattern.substr(i + 1, digit_amount).c_str());
				i += digit_amount;
			}
		}

		else if (pattern[i] == L'*' || pattern[i] == L'#')
		{
			style = pattern[i] == L'*' ? 1 : 2;
			bool wrong = false;

			if (first)
			{
				first = false;

				for (; current < name.length(); ++current)
				{
					if (iswdigit(name[current]))
					{
						start = current;
						break;
					}
				}

				// This shouldn't happen, means there are no numbers whatsoever in the filename
				if (start == -1) return 0;
			}

			for (int j = current; j <= name.length(); ++j)
			{
				if (j < name.length() && iswdigit(name[j]) && amount != 0) amount--;

				// Restart looking for this pattern
				else if (amount > 0 || j == current)
				{
					wrong = true;
					break;
				}

				else
				{
					if (start == -1) start = 0;
					int value = _wtoi(name.substr(current, j - current).c_str());
					current = j;
					if (style == 1) ep.season = value;
					else ep.episode = value;
					break;
				}
			}

			if (wrong)
			{
				// Or pattern doesn't exist in this name
				if (++start >= name.length() - 1) return 0;

				current = start;
				i = -1;
			}
		}

		else
		{
			bool found = false;

			if (i == 0)
			{
				for (; current < name.length(); ++current)
				{
					if (towlower(name[current]) == towlower(pattern[i]))
					{
						start = current;
						current++;
						found = true;
						break;
					}
				}
			}

			else
			{
				if (towlower(name[current]) == towlower(pattern[i]))
				{
					current++;
					found = true;
				}
			}

			// Restart looking for this pattern
			if (!found)
			{
				// Or start the next pattern
				if (start == -1 || (++start >= name.length() - 1)) return 0;

				current = start;
				i = -1;
			}
		}
	}

	return current;
}