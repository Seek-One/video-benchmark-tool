#include "Benchmark.h"

#include <cassert>
#include <clocale>
#include <ctime>
#include <fstream>
#include <mutex>

#include <json.hpp>

#include "Helper/AVCodecHelper.h"
#include "Helper/AVFormatHelper.h"
#include "Helper/Log.h"

#include "Configuration.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace vmaf {
	const std::filesystem::path Benchmark::DumpDir = "dump";

	Benchmark::Benchmark()
	: m_iThreadCount(std::thread::hardware_concurrency())
	{

	}

	void Benchmark::setThreadCount(unsigned iCount)
	{
		m_iThreadCount = iCount;
	}

	void Benchmark::reset()
	{
		m_poolThreads.clear();
		m_results.clear();
	}

	void Benchmark::abort()
	{
		// Abort all thread
		for (auto& thread: m_poolThreads) {
			thread.abort();
		}
	}

	void Benchmark::start(
		const std::string& szVideoFileName,
		const CodecList& listCodec,
		int iMinCRF,
		int iMaxCRF,
		const std::vector<std::string>& listPreset,
		std::function<void()> callback
	)
	{
		helper::Log::info("selected video: %s", szVideoFileName.c_str());
		helper::Log::info("selected codec:");
		for (const auto& codecID: listCodec) {
			helper::Log::info("\t%s", types::getCodecName(codecID));
		}
		helper::Log::info("CRF: [%d - %d]", iMinCRF, iMaxCRF);
		helper::Log::info("selected preset:");
		for (const auto& szPreset: listPreset) {
			helper::Log::info("\t%s", szPreset.c_str());
		}

		types::PacketList yuvFrames;
		if (!decodeOriginalVideoFile(szVideoFileName, yuvFrames)) {
			helper::Log::error("Error decoding...");
		}

		// Create video dump directory
		fs::remove_all(DumpDir);
		fs::create_directory(DumpDir);

		// Dump original video
		std::ofstream dumpFile(DumpDir / "original-video.yuv", std::ios::binary);
		assert(dumpFile.good());

		for (const auto& yuvFrame: yuvFrames) {
			dumpFile.write(reinterpret_cast<const char*>(yuvFrame.data()), yuvFrame.size());
		}

		runExperiments(yuvFrames, listCodec, iMinCRF, iMaxCRF, listPreset, callback);
	}

	bool Benchmark::decodeOriginalVideoFile(const std::string& szVideoFileName, types::PacketList& yuvFrames)
	{
		helper::avcodec::Context codecContex;

		if (codecContex.decodeVideoFile(szVideoFileName.c_str(), yuvFrames) != helper::avcodec::Error::Success) {
			helper::Log::error("Error decode video...");
			return false;
		}

		m_originalCodecParameters = codecContex.getCodecParameters();

		return true;
	}

	void Benchmark::runExperiments(
		const types::PacketList& yuvFrames,
		const CodecList& listCodec,
		int iMinCRF,
		int iMaxCRF,
		const std::vector<std::string>& listPreset,
		std::function<void()> callback
	)
	{
		std::vector<Configuration> listConfigurations;

		// Generate all configuration
		for (const auto& codecID: listCodec) {
			for (int iCRF = iMinCRF; iCRF <= iMaxCRF; ++iCRF) {
				for (const auto& szPreset: listPreset) {
					listConfigurations.push_back({ codecID, iCRF, szPreset });
				}
			}
		}

		// Keep previous locale
		std::string szCurrentNumericLocale = std::setlocale(LC_NUMERIC, nullptr);
		std::string szCurrentTimeLocale = std::setlocale(LC_TIME, nullptr);

		// Set locale to 'C' to avoid bug during vmaf model loading
		// Must be done before threads launch since it's not thread-safe
		std::setlocale(LC_NUMERIC, "C");
		std::setlocale(LC_TIME, "C");

		// Alloc the thread pool
		std::mutex mutexExperiments;
		for (int i = 0; i < static_cast<int>(m_iThreadCount); ++i) {
			m_poolThreads.emplace_back(yuvFrames, m_originalCodecParameters, listConfigurations, mutexExperiments, callback);
		}

		// Start all threads
		for (auto& thread: m_poolThreads) {
			thread.start();
		}

		// Join all thread
		for (auto& thread: m_poolThreads) {
			thread.wait();
			auto threadResults = thread.getResults();
			m_results.insert(threadResults.begin(), threadResults.end());
		}

		// Print results
		json jsonDocument;
		for (const auto& [key, value]: m_results) {
			helper::Log::debug("[Codec=%d, CRF=%d, preset=%s]: VMAF=%f", static_cast<int>(key.codecType), key.iCRF, key.szPreset.c_str(), value.dVMAFScore);
			json jKey = {
				"key", {
					{ "codec_id", static_cast<int>(key.codecType) },
					{ "codec_name", types::getCodecName(key.codecType) },
					{ "crf", key.iCRF },
					{ "preset", key.szPreset.c_str() }
				}
			};

			json jResult = {
				"results", {
					{ "vmaf", value.dVMAFScore },
					{ "bitstream_size", value.iBitstreamSize },
					{ "encoding_time", value.dEncodingTime },
					{ "decoding_time", value.dDecdodingTime },
				}
			};
			jsonDocument["experiments"].push_back(
				{ jKey, jResult }
			);
		}

		std::ofstream jsonFile;

		std::array<char, 256> dateTimeText = { 0 };

		std::time_t currentTime = std::time(nullptr);
		std::strftime(dateTimeText.data(), dateTimeText.size(), "%Y-%m-%d-%H%M%S-results.json", std::localtime(&currentTime));
		jsonFile.open(dateTimeText.data());

		if (!jsonFile.good()) {
			helper::Log::error("Unable to store results :\n%s", jsonDocument.dump(4).c_str());
			return;
		}

		jsonFile << jsonDocument;

		// Restore the previous locale
		std::setlocale(LC_NUMERIC, szCurrentNumericLocale.c_str());
		std::setlocale(LC_TIME, szCurrentTimeLocale.c_str());
	}
}
