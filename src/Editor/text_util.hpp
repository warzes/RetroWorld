#pragma once

#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

namespace ed
{
	inline std::string BuildPath(std::initializer_list<std::string> components)
	{
		std::string output;
		for (const std::string& s : components)
		{
			output += s;
			if (&s != &*(components.end() - 1) && s.back() != '/')
				output += '/';
		}
		return output;
	}

	inline std::vector<std::string> SplitString(const std::string& input, const std::string& delimiter)
	{
		std::vector<std::string> result;
		size_t tokenStart = 0, tokenEnd = 0;
		while ((tokenEnd = input.find(delimiter, tokenStart)) != std::string::npos)
		{
			result.push_back(input.substr(tokenStart, tokenEnd - tokenStart));
			tokenStart = tokenEnd + delimiter.length();
		}
		if (tokenStart < input.length())
			result.push_back(input.substr(tokenStart));
		return result;
	}

	inline std::string StringToLower(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return str;
	}
} // namespace ed