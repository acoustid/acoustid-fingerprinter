/*
 * Chromaprint -- Audio fingerprinting toolkit
 * Copyright (C) 2010  Lukas Lalinsky <lalinsky@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef FPSUBMIT_DECODER_H_
#define FPSUBMIT_DECODER_H_

#include <QMutex>
#include <string>
#include <algorithm>
#include <stdint.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#ifdef HAVE_AV_AUDIO_CONVERT
#include "ffmpeg/audioconvert.h"
#include "ffmpeg/samplefmt.h"
#endif
}
#include "fingerprintcalculator.h"

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 64, 0)
#define AV_SAMPLE_FMT_S16 SAMPLE_FMT_S16
#endif

class Decoder
{
public:
	Decoder(const std::string &fileName);
	~Decoder();

	bool Open();
	void Decode(FingerprintCalculator *consumer, int maxLength = 0);

	int Channels()
	{
		return m_codec_ctx->channels;
	}

	int SampleRate()
	{
		return m_codec_ctx->sample_rate;
	}

	std::string LastError()
	{
		return m_error;
	}

    //static void lock_manager();
    static void initialize();

private:
	static const int BUFFER_SIZE = AVCODEC_MAX_AUDIO_FRAME_SIZE * 2;
	uint8_t *m_buffer1;
	uint8_t *m_buffer2;
	std::string m_file_name;
	std::string m_error;
	AVFormatContext *m_format_ctx;
	AVCodecContext *m_codec_ctx;
	bool m_codec_open;
	AVStream *m_stream;
    static QMutex m_mutex;
#ifdef HAVE_AV_AUDIO_CONVERT
	AVAudioConvert *m_convert_ctx;
#endif
};

/*inline static void Decoder::lock_manager(void **mutex, enum AVLockOp op)
{
    switch (op) {
    case AV_LOCK_CREATE:
        *mutex = new QMutex();
        return 1;
    case AV_LOCK_DESTROY:
        delete (QMutex *)(*mutex);
        return 1;
    case AV_LOCK_ACQUIRE:
        ((QMutex *)(*mutex))->lock();
        return 1;
    case AV_LOCK_RELEASE:
        ((QMutex *)(*mutex))->unlock();
        return 1;
    }
    return 0;
}*/

inline void Decoder::initialize()
{
	av_register_all();
	av_log_set_level(AV_LOG_ERROR);
    //av_lockmgr_register(&Decoder::lock_manager)
}

inline Decoder::Decoder(const std::string &file_name)
	: m_file_name(file_name), m_format_ctx(0), m_codec_ctx(0), m_stream(0), m_codec_open(false)
#ifdef HAVE_AV_AUDIO_CONVERT
	, m_convert_ctx(0)
#endif
{
	m_buffer1 = (uint8_t *)av_malloc(BUFFER_SIZE + 16);
	m_buffer2 = (uint8_t *)av_malloc(BUFFER_SIZE + 16);
}

inline Decoder::~Decoder()
{
	if (m_codec_ctx && m_codec_open) {
        QMutexLocker locker(&m_mutex); 
		avcodec_close(m_codec_ctx);
	}
	if (m_format_ctx) {
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53, 17, 0)
		av_close_input_file(m_format_ctx);
#else
		avformat_close_input(&m_format_ctx);
#endif
	}
#ifdef HAVE_AV_AUDIO_CONVERT
	if (m_convert_ctx) {
		av_audio_convert_free(m_convert_ctx);
	}
#endif
	av_free(m_buffer1);
	av_free(m_buffer2);
}

inline bool Decoder::Open()
{
    QMutexLocker locker(&m_mutex); 

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53, 2, 0)
	if (av_open_input_file(&m_format_ctx, m_file_name.c_str(), NULL, 0, NULL) != 0) {
#else
	if (avformat_open_input(&m_format_ctx, m_file_name.c_str(), NULL, NULL) != 0) {
#endif
		m_error = "Couldn't open the file." + m_file_name;
		return false;
	}

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53, 6, 0)
	if (av_find_stream_info(m_format_ctx) < 0) {
#else
	if (avformat_find_stream_info(m_format_ctx, NULL) < 0) {
#endif
		m_error = "Couldn't find stream information in the file.";
		return false;
	}

	//dump_format(m_format_ctx, 0, m_file_name.c_str(), 0);

	for (int i = 0; i < m_format_ctx->nb_streams; i++) {
		AVCodecContext *avctx = m_format_ctx->streams[i]->codec;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 64, 0)
                if (avctx && avctx->codec_type == CODEC_TYPE_AUDIO) {
#else
                if (avctx && avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
#endif
			m_stream = m_format_ctx->streams[i];
			m_codec_ctx = avctx;
			break;
		}
	}
	if (!m_codec_ctx) {
		m_error = "Couldn't find any audio stream in the file.";
		return false;
	}

	AVCodec *codec = avcodec_find_decoder(m_codec_ctx->codec_id);
	if (!codec) {
		m_error = "Unknown codec.";
		return false;
	}

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 8, 0)
    if (avcodec_open(m_codec_ctx, codec) < 0) {
#else
	if (avcodec_open2(m_codec_ctx, codec, NULL) < 0) {
#endif
        m_error = "Couldn't open the codec.";
        return false;
    }
	m_codec_open = true;

	if (m_codec_ctx->sample_fmt != AV_SAMPLE_FMT_S16) {
#ifdef HAVE_AV_AUDIO_CONVERT
		m_convert_ctx = av_audio_convert_alloc(AV_SAMPLE_FMT_S16, m_codec_ctx->channels,
		                                       (AVSampleFormat)m_codec_ctx->sample_fmt, m_codec_ctx->channels, NULL, 0);
		if (!m_convert_ctx) {
			m_error = "Couldn't create sample format converter.";
			return false;
		}
#else
		m_error = "Unsupported sample format.";
		return false;
#endif
	}

	if (Channels() <= 0) {
		m_error = "Invalid audio stream (no channels).";
		return false;
	}

	if (SampleRate() <= 0) {
		m_error = "Invalid sample rate.";
		return false;
	}

	return true;
}

#include <stdio.h>

inline void Decoder::Decode(FingerprintCalculator *consumer, int max_length)
{
	AVPacket packet, packet_temp;

	int remaining = max_length * SampleRate() * Channels();
	int stop = 0;

	av_init_packet(&packet);
	av_init_packet(&packet_temp);
	while (!stop) {
		if (av_read_frame(m_format_ctx, &packet) < 0) {
	//		consumer->Flush();	
			break;
		}

		packet_temp.data = packet.data;
		packet_temp.size = packet.size;
		while (packet_temp.size > 0) {
			int buffer_size = BUFFER_SIZE;
#if LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(52, 25, 0)
                        int consumed = avcodec_decode_audio2(
                                m_codec_ctx, (int16_t *)m_buffer1, &buffer_size,
                                packet_temp.data, packet_temp.size);
#else
                        int consumed = avcodec_decode_audio3(
                                m_codec_ctx, (int16_t *)m_buffer1, &buffer_size,
                                &packet_temp);
#endif

			if (consumed < 0) {
				break;
			}

			packet_temp.data += consumed;
			packet_temp.size -= consumed;

			if (buffer_size <= 0) {
				continue;
			}

			int16_t *audio_buffer;
#ifdef HAVE_AV_AUDIO_CONVERT
			if (m_convert_ctx) {
				const void *ibuf[6] = { m_buffer1 };
				void *obuf[6] = { m_buffer2 };
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51, 8, 0)
				int istride[6] = { av_get_bits_per_sample_format(m_codec_ctx->sample_fmt) / 8 };
#else
				int istride[6] = { av_get_bytes_per_sample(m_codec_ctx->sample_fmt) };
#endif
				int ostride[6] = { 2 };
				int len = buffer_size / istride[0];
				if (av_audio_convert(m_convert_ctx, obuf, ostride, ibuf, istride, len) < 0) {
					break;
				}
				audio_buffer = (int16_t *)m_buffer2;
				buffer_size = len * ostride[0];
			}
			else {
				audio_buffer = (int16_t *)m_buffer1;
			}
#else
			audio_buffer = (int16_t *)m_buffer1;
#endif

			int length = buffer_size / 2;
			if (max_length) {
				length = std::min(remaining, length);
			}

			consumer->feed(audio_buffer, length);

			if (max_length) {
				remaining -= length;
				if (remaining <= 0) {
					stop = 1;
					break;
				}
			}
		}

		if (packet.data) {
			av_free_packet(&packet);
		}
	}
}

#endif
