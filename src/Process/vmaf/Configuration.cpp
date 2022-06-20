/* Copyright (c) 2022 Jet1oeil
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "Configuration.h"

#include <filesystem>
#include <fstream>
#include <tuple>

#include <json.hpp>

#include "Helper/Log.h"

namespace fs = std::filesystem;

using json = nlohmann::json;

namespace vmaf {
	bool Configuration::operator<(const Configuration& other) const
	{
		std::tuple<int, int, std::string, int> tuple{ static_cast<int>(codecType), iCRF, szPreset, iBitrate };
		std::tuple<int, int, std::string, int> otherTuple{ static_cast<int>(other.codecType), other.iCRF, other.szPreset, other.iBitrate };

		return tuple < otherTuple;
	}

	bool updateConfigurationListFile(const std::vector<Configuration> &listConfigurations)
	{
		json jDocument;

		std::ifstream configFile(fs::temp_directory_path() / "vmaf-benchmark-configs.json");

		if (!configFile.good()) {
			return false;
		}

		configFile >> jDocument;

		std::string videoSource = jDocument["video_source"];

		return writeConfigurationList(videoSource, listConfigurations);
	}

	bool writeConfigurationList(const std::string& videoFile, const std::vector<Configuration> &listConfigurations)
	{
		json jDocument;

		jDocument["video_source"] = videoFile;

		// Dump configuration list to a json file
		for (const auto& configuration: listConfigurations) {
			jDocument["configurations"].push_back(
				{
					{ "codec_type", configuration.codecType },
					{ "bitrate", configuration.iBitrate },
					{ "crf", configuration.iCRF },
					{ "preset", configuration.szPreset }
				}
			);
		}

		std::ofstream configFile(fs::temp_directory_path() / "vmaf-benchmark-configs.json");

		if (!configFile.good()) {
			return false;
		}

		configFile << jDocument;

		helper::Log::debug("%s", jDocument.dump(4).c_str());

		return true;
	}
}
