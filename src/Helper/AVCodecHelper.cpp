#include "AVCodecHelper.h"

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

#include <cassert>
#include <cstring>

#include "AVFormatHelper.h"
#include "Log.h"

namespace helper {
	namespace avcodec {
		bool Context::s_bCallbackDefined = false;
		std::mutex Context::s_logMutext;

		namespace details {
			types::PixelFormat convertPixelFormat(AVPixelFormat pixelFormat)
			{
				switch (pixelFormat) {
				// case AV_PIX_FMT_GRAY8:
				// case AV_PIX_FMT_GRAY9:
				// case AV_PIX_FMT_GRAY10:
				// case AV_PIX_FMT_GRAY12:
				// case AV_PIX_FMT_GRAY14:
				// case AV_PIX_FMT_GRAY16:

				case AV_PIX_FMT_YUV420P:
				case AV_PIX_FMT_YUVJ420P:
				case AV_PIX_FMT_YUV420P9:
				case AV_PIX_FMT_YUV420P10:
				case AV_PIX_FMT_YUV420P12:
				case AV_PIX_FMT_YUV420P14:
				case AV_PIX_FMT_YUV420P16:
					return types::PixelFormat::YUV420P;

				// case AV_PIX_FMT_YUV422P:
				// case AV_PIX_FMT_YUV422P9:
				// case AV_PIX_FMT_YUV422P10:
				// case AV_PIX_FMT_YUV422P12:
				// case AV_PIX_FMT_YUV422P14:
				// case AV_PIX_FMT_YUV422P16:


				// case AV_PIX_FMT_YUV444P:
				// case AV_PIX_FMT_YUV444P9:
				// case AV_PIX_FMT_YUV444P10:
				// case AV_PIX_FMT_YUV444P12:
				// case AV_PIX_FMT_YUV444P14:
				// case AV_PIX_FMT_YUV444P16:

				default:
					return types::PixelFormat::Undefined;
				}

				return types::PixelFormat::Undefined;
			}

			AVPixelFormat convertPixelFormat(types::PixelFormat pixelFormat)
			{
				switch (pixelFormat) {
				case types::PixelFormat::YUV420P:
					return AV_PIX_FMT_YUV420P;

				case types::PixelFormat::Undefined:
					return AV_PIX_FMT_NONE;
				}

				return AV_PIX_FMT_NONE;
			}

			types::ColorRange convertColorRange(AVColorRange colorRange)
			{
				switch (colorRange) {
				case AVCOL_RANGE_MPEG:
					return types::ColorRange::MPEG;

				case AVCOL_RANGE_JPEG:
					return types::ColorRange::JPEG;

				default:
					return types::ColorRange::Undefined;
				}

				return types::ColorRange::Undefined;
			}

			AVColorRange convertColorRange(types::ColorRange colorRange)
			{
				switch (colorRange) {
				case types::ColorRange::MPEG:
					return AVCOL_RANGE_MPEG;

				case types::ColorRange::JPEG:
					return AVCOL_RANGE_JPEG;

				case types::ColorRange::Undefined:
					return AVCOL_RANGE_UNSPECIFIED;
				}

				return AVCOL_RANGE_UNSPECIFIED;
			}

			int getPixelDepth(AVPixelFormat pixelFormat)
			{
				switch (pixelFormat) {
				case AV_PIX_FMT_GRAY8:
				case AV_PIX_FMT_YUV420P:
				case AV_PIX_FMT_YUVJ420P:
				case AV_PIX_FMT_YUV422P:
				case AV_PIX_FMT_YUV444P:
					return 8;

				// case AV_PIX_FMT_GRAY9:
				// case AV_PIX_FMT_YUV420P9:
				// case AV_PIX_FMT_YUV422P9:
				// case AV_PIX_FMT_YUV444P9:
				// 	return 9;

				// case AV_PIX_FMT_GRAY10:
				// case AV_PIX_FMT_YUV420P10:
				// case AV_PIX_FMT_YUV422P10:
				// case AV_PIX_FMT_YUV444P10:
				// 	return 10;

				// case AV_PIX_FMT_GRAY12:
				// case AV_PIX_FMT_YUV420P12:
				// case AV_PIX_FMT_YUV422P12:
				// case AV_PIX_FMT_YUV444P12:
				// 	return 12;

				// case AV_PIX_FMT_GRAY14:
				// case AV_PIX_FMT_YUV420P14:
				// case AV_PIX_FMT_YUV444P14:
				// case AV_PIX_FMT_YUV422P14:
				// 	return 14;

				// case AV_PIX_FMT_GRAY16:
				// case AV_PIX_FMT_YUV420P16:
				// case AV_PIX_FMT_YUV422P16:
				// case AV_PIX_FMT_YUV444P16:
				// 	return 16;

				default:
					return -1;
				}

				return -1;
			}

			AVCodecID getCodecID(types::CodecType codec)
			{
				switch (codec) {
				case types::CodecType::H264Baseline:
				case types::CodecType::H264Main:
				case types::CodecType::H264High:
					return AV_CODEC_ID_H264;

				case types::CodecType::H265Main:
					return AV_CODEC_ID_HEVC;

				case types::CodecType::Undefined:
					return AV_CODEC_ID_NONE;
				}

				return AV_CODEC_ID_NONE;
			}

			int getProfileID(types::CodecType codec)
			{
				switch (codec) {
				case types::CodecType::H264Baseline:
					return FF_PROFILE_H264_BASELINE;

				case types::CodecType::H264Main:
					return FF_PROFILE_H264_MAIN;

				case types::CodecType::H264High:
					return FF_PROFILE_H264_HIGH;

				case types::CodecType::H265Main:
					return FF_PROFILE_HEVC_MAIN;

				case types::CodecType::Undefined:
					return FF_PROFILE_UNKNOWN;
				}

				return FF_PROFILE_UNKNOWN;
			}

			void customAVLogger([[ maybe_unused ]] void* avcl, int level, const char *fmt, va_list vl)
			{
				int iCurrentLevel = av_log_get_level();
				if (level > iCurrentLevel) {
					return;
				}

				// Remove last end line
				std::string strFmt(fmt);
				strFmt.pop_back();

				// Get the class name
				AVClass* avPtr = (avcl ? *(static_cast<AVClass**>(avcl)) : nullptr);
				if (avPtr != nullptr) {
					std::string avClassName(avPtr->item_name(avcl));
					strFmt = avClassName + " " + strFmt;
				}

				static std::mutex mutex;
				std::lock_guard<std::mutex> lock(mutex);

				switch (level) {
				case AV_LOG_QUIET:
					// Nothing to do
					break;

				case AV_LOG_VERBOSE:
				case AV_LOG_DEBUG:
				case AV_LOG_TRACE:
					Log::debug(strFmt.c_str(), vl);
					break;

				case AV_LOG_INFO:
					Log::info(strFmt.c_str(), vl);
					break;

				case AV_LOG_WARNING:
					Log::warning(strFmt.c_str(), vl);
					break;

				case AV_LOG_ERROR:
					Log::error(strFmt.c_str(), vl);
					break;

				case AV_LOG_PANIC:
				case AV_LOG_FATAL:
					Log::fatal(strFmt.c_str(), vl);
					break;

				default:
					assert(false);
					break;
				}
			}
		}

		Context::Context()
		: m_pContext(nullptr)
		, m_pPacket(nullptr)
		, m_pFrame(nullptr)
		{
			{
				std::lock_guard<std::mutex> locker(s_logMutext);

				if (!s_bCallbackDefined) {
#if DEBUG
					av_log_set_level(AV_LOG_INFO);
#else
					av_log_set_level(AV_LOG_WARNING);
#endif
					av_log_set_callback(details::customAVLogger);

					s_bCallbackDefined = true;
				}
			}
		}

		Context::~Context()
		{
			if (m_pContext != nullptr) {
				avcodec_free_context(&m_pContext);
			}

			if (m_pPacket != nullptr) {
				av_packet_free(&m_pPacket);
			}

			if (m_pFrame != nullptr) {
				av_frame_free(&m_pFrame);
			}
		}

		types::CodecParameters Context::getCodecParameters() const
		{
			types::CodecParameters parameters;

			parameters.videoSize = { m_pContext->width, m_pContext->height };
			parameters.pixelFormat = details::convertPixelFormat(m_pContext->pix_fmt);
			parameters.colorRange = details::convertColorRange(m_pContext->color_range);
			parameters.iPixelDepth = details::getPixelDepth(m_pContext->pix_fmt);

			parameters.iFPS = m_pContext->time_base.den;

			return parameters;
		}

		Error Context::decodeVideoFile(const char* szVideoFileName, types::PacketList& yuvFrames)
		{
			avformat::Context formatContext;

			if (formatContext.openFile(szVideoFileName) != avformat::Error::Success) {
				return Error::OpenCodec;
			}

			Error error = openDecoder(formatContext);
			if (error != avcodec::Error::Success) {
				return Error::OpenCodec;
			}

			avcodec::Error errorCodec = avcodec::Error::Success;
			do {
				errorCodec = decodePacket(formatContext, yuvFrames);
			} while (errorCodec == avcodec::Error::Success);

			if (errorCodec != avcodec::Error::CodecFlushed) {
				return Error::Unkown;
			}

			return Error::Success;
		}

		Error Context::decodePacketStream(types::PacketList& packets, types::CodecType codecType, types::PacketList& yuvFrames)
		{
			Error error = Error::Success;

			if (openDecoder(codecType) != avcodec::Error::Success) {
				return Error::OpenCodec;
			}

			for (auto& packet: packets) {
				m_pPacket->data = reinterpret_cast<uint8_t*>(packet.data());
				m_pPacket->size = packet.size();

				if (decodeVideoFrame(m_pPacket, yuvFrames) != avcodec::Error::Success) {
					return Error::Unkown;
				}
			}

			// Flush decoder
			if (decodeVideoFrame(nullptr, yuvFrames) != avcodec::Error::CodecFlushed) {
				return Error::Unkown;
			}

			return error;
		}

		Error Context::encodeFrameStream(
			const types::PacketList& yuvFrames,
			const types::CodecParameters& parameters,
			const types::EncoderParameters& encoderParameters,
			types::PacketList& packets
		)
		{
			if (openEncoder(parameters, encoderParameters) != avcodec::Error::Success) {
				return Error::OpenCodec;
			}

			for (const auto& yuvFrame: yuvFrames) {
				if (encodeFrame(yuvFrame, packets) != avcodec::Error::Success) {
					return Error::Unkown;
				}
			}

			// Flush decoder
			if (encodeFrame(types::Packet(), packets) != avcodec::Error::CodecFlushed) {
				return Error::Unkown;
			}

			return Error::Success;
		}

		Error Context::allocateContext(const AVCodec* pCodec)
		{
			m_pPacket = av_packet_alloc();
			if (m_pPacket == nullptr) {
				return Error::NoMemory;
			}

			m_pFrame = av_frame_alloc();
			if (m_pFrame == nullptr) {
				return Error::NoMemory;
			}

			m_pContext = avcodec_alloc_context3(pCodec);
			if (m_pContext == nullptr) {
				return Error::NoMemory;
			}

			return Error::Success;
		}

		Error Context::openDecoder(const avformat::Context& formatContext)
		{
			const AVCodec* pCodec = formatContext.getCodec();

			if (allocateContext(pCodec) != Error::Success) {
				return Error::NoMemory;
			}

			auto videoStream = formatContext.getVideoStream();
			if (avcodec_parameters_to_context(m_pContext, videoStream.pStream->codecpar) < 0) {
				return Error::CopyParameters;
			}

			if (avcodec_open2(m_pContext, pCodec, nullptr) < 0) {
				return Error::OpenCodec;
			}

			return Error::Success;
		}

		Error Context::openDecoder(types::CodecType codecType)
		{
			AVCodec* pCodec = avcodec_find_decoder(details::getCodecID(codecType));

			if (allocateContext(pCodec) != Error::Success) {
				return Error::NoMemory;
			}

			if (pCodec == nullptr) {
				return Error::NoCodecFound;
			}

			if (avcodec_open2(m_pContext, pCodec, nullptr) < 0) {
				return Error::OpenCodec;
			}

			return Error::Success;
		}

		Error Context::decodeVideoFrame(const AVPacket* pPacket, types::PacketList& yuvFrames)
		{
			if (avcodec_send_packet(m_pContext, pPacket) < 0) {
				return Error::SendPacket;
			}

			int iRes = 0;
			while (iRes >= 0) {
				iRes = avcodec_receive_frame(m_pContext, m_pFrame);
				if (iRes == AVERROR(EAGAIN)) {
					return Error::Success;
				} else if (iRes == AVERROR_EOF) {
					return Error::CodecFlushed;
				} else if (iRes < 0) {
					return Error::ReceiveFrame;
				}

				// Dump yuv
				types::Packet yuvBytes;
				yuvBytes.insert(yuvBytes.end(), m_pFrame->data[0], m_pFrame->data[0] + m_pFrame->width * m_pFrame->height);
				yuvBytes.insert(yuvBytes.end(), m_pFrame->data[1], m_pFrame->data[1] + m_pFrame->width * m_pFrame->height / 4);
				yuvBytes.insert(yuvBytes.end(), m_pFrame->data[2], m_pFrame->data[2] + m_pFrame->width * m_pFrame->height / 4);

				yuvFrames.push_back(yuvBytes);
			}

			return Error::Success;
		}

		Error Context::decodePacket(avformat::Context& formatContext, types::PacketList& yuvFrames)
		{
			Error codecError = Error::Success;
			auto formatError = formatContext.readVideoFrame(*m_pPacket);

			if (formatError == avformat::Error::Success) {
				codecError = decodeVideoFrame(m_pPacket, yuvFrames);
			} else if (formatError == avformat::Error::EndOfFile) {
				codecError = decodeVideoFrame(nullptr, yuvFrames);
			} else {
				codecError = Error::Unkown;
			}

			av_packet_unref(m_pPacket);

			return codecError;
		}

		Error Context::openEncoder(const types::CodecParameters& parameters, const types::EncoderParameters& encoderParameters)
		{
			// Only support 8-bits
			assert(parameters.iPixelDepth == 8);

			const AVCodec* pCodec = avcodec_find_encoder(details::getCodecID(encoderParameters.codecType));
			if (pCodec == nullptr) {
				return Error::NoCodecFound;
			}

			if (allocateContext(pCodec) != Error::Success) {
				return Error::NoMemory;
			}

			m_pContext->profile = details::getProfileID(encoderParameters.codecType);

			m_pContext->time_base = { 1, parameters.iFPS };
			m_pContext->framerate = { parameters.iFPS, 1 };

			m_pContext->pix_fmt = details::convertPixelFormat(parameters.pixelFormat);
			m_pContext->color_range = details::convertColorRange(parameters.colorRange);

			m_pContext->width = parameters.videoSize.width;
			m_pContext->height = parameters.videoSize.height;

			m_pContext->thread_count = 1;

			AVDictionary* options = nullptr;
			av_dict_set(&options, "crf", std::to_string(encoderParameters.iCRF).c_str(), 0);
			av_dict_set(&options, "preset", encoderParameters.szPreset.c_str(), 0);

			if (encoderParameters.codecType == types::CodecType::H265Main) {
				av_dict_set(&options, "x265-params", "--pools=none, --numa-pools=none", 0);
			}

			if (avcodec_open2(m_pContext, pCodec, &options) < 0) {
				return Error::OpenCodec;
			}

			m_pFrame->format = m_pContext->pix_fmt;
			m_pFrame->color_range = m_pContext->color_range;
			m_pFrame->width = parameters.videoSize.width;
			m_pFrame->height = parameters.videoSize.height;

			if (av_frame_get_buffer(m_pFrame, 0) < 0) {
				return Error::NoMemory;
			}

			assert(m_pContext->thread_count == 1);

			return Error::Success;
		}

		Error Context::encodeVideoFrame(const AVFrame* pFrame, types::PacketList& packets)
		{
			if (avcodec_send_frame(m_pContext, pFrame) < 0) {
				return Error::SendFrame;
			}

			int iRes = 0;
			while (iRes >= 0) {
				iRes = avcodec_receive_packet(m_pContext, m_pPacket);
				if (iRes == AVERROR(EAGAIN)) {
					return Error::Success;
				} else if (iRes == AVERROR_EOF) {
					return Error::CodecFlushed;
				} else if (iRes < 0) {
					return Error::ReceiveFrame;
				}

				// Dump yuv
				types::Packet packetBytes(m_pPacket->data, m_pPacket->data + m_pPacket->size);
				packets.push_back(packetBytes);

				av_packet_unref(m_pPacket);
			}

			return Error::Success;
		}

		Error Context::encodeFrame(const types::Packet& yuvFrame, types::PacketList& packets)
		{
			if (yuvFrame.empty()) {
				return encodeVideoFrame(nullptr, packets);
			}

			// Fill AVFrame
			const std::uint8_t* pSrc = yuvFrame.data();
			std::memcpy(m_pFrame->data[0], pSrc, m_pFrame->width * m_pFrame->height);
			pSrc += m_pFrame->width * m_pFrame->height;
			std::memcpy(m_pFrame->data[1], pSrc, m_pFrame->width * m_pFrame->height / 4);
			pSrc += m_pFrame->width * m_pFrame->height / 4;
			std::memcpy(m_pFrame->data[2], pSrc, m_pFrame->width * m_pFrame->height / 4);

			return encodeVideoFrame(m_pFrame, packets);
		}
	}
}