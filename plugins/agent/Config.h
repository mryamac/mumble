#pragma once
#include <string>

#define CONFIG_FILE_PATH "/app/config.ini"

class Config {
private:
	std::string mumbleConnectionString;

public:
	Config();
	~Config();

	std::string getMumbleConnectionString();
};