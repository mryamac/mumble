#include "Config.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <unordered_map>

Config::Config() {
	std::ifstream stream(CONFIG_FILE_PATH);
	if (!stream.is_open())
		logd(LOG_CRIT, "Failed to open config file");

	std::unordered_map< std::string, std::string > keyValuePairs;

	// Read and parse each line from the file
	std::string line;
	while (std::getline(stream, line)) {
		std::istringstream iss(line);
		std::string key, value;
		if (std::getline(iss, key, '=') && std::getline(iss, value)) {
			// Trim leading and trailing whitespaces from key and value
			key.erase(0, key.find_first_not_of(" \t\n\r\f\v"));
			key.erase(key.find_last_not_of(" \t\n\r\f\v") + 1);
			value.erase(0, value.find_first_not_of(" \t\n\r\f\v"));
			value.erase(value.find_last_not_of(" \t\n\r\f\v") + 1);

			// Insert key-value pair into the map
			keyValuePairs[key] = value;
		}
	}

	this->mumbleConnectionString = keyValuePairs["mumbleConnectionServerString"];
	if (this->mumbleConnectionString.empty())
		logd(LOG_ERR, "mumbleConnectionServerString is not set in the config file");
}

Config::~Config() {
}

std::string Config::getMumbleConnectionString() {
	return this->mumbleConnectionString;
}