/*************************************************************************/
/*  opus_codec.h                                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef OPUS_CODEC_HPP
#define OPUS_CODEC_HPP

#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>

#include "speech_decoder.h"

#include "thirdparty/opus/opus/opus.h"

#include "macros.h"

#include "ogg_packer.h"

#if SPEECH_DECODER_POLYMORPHISM
class OpusSpeechDecoder : public SpeechDecoder {
	GDCLASS(OpusSpeechDecoder, SpeechDecoder);
	::OpusDecoder *decoder = NULL;

public:
	OpusSpeechDecoder() {
	}
	~OpusSpeechDecoder() {
		set_decoder(NULL);
	}

	void _init() {}

	void set_decoder(::OpusDecoder *p_decoder) {
		if (!decoder) {
			opus_decoder_destroy(decoder);
		}
		decoder = p_decoder;
	}

	virtual bool process(
			const PackedByteArray *p_compressed_buffer,
			PackedByteArray *p_pcm_output_buffer,
			const int p_compressed_buffer_size,
			const int p_pcm_output_buffer_size,
			const int p_buffer_frame_count) {
		if (decoder) {
			opus_int16 *output_buffer_pointer = reinterpret_cast<opus_int16 *>(p_pcm_output_buffer->write().ptr());
			const unsigned char *opus_buffer_pointer = reinterpret_cast<const unsigned char *>(p_compressed_buffer->read().ptr());

			opus_int32 ret_value = opus_decode(decoder, opus_buffer_pointer, p_compressed_buffer_size, output_buffer_pointer, p_buffer_frame_count, 0);
			return true;
		}

		return false;
	}
};
#endif

// TODO: always assumes little endian

template <uint32_t SAMPLE_RATE, uint32_t CHANNEL_COUNT>
class OpusCodec {
private:
	static const uint32_t APPLICATION = OPUS_APPLICATION_VOIP;

	static const int BUFFER_FRAME_COUNT = SAMPLE_RATE / 100; //MILLISECONDS_PER_PACKET;

	static const int INTERNAL_BUFFER_SIZE = (100 * 3 * 1276);
	unsigned char internal_buffer[INTERNAL_BUFFER_SIZE];

	OpusEncoder *encoder = NULL;
	OpusDecoder *xxdecoder = NULL;
	oggpacker *oggp = NULL;
	FILE *ogg_output_file = NULL;
	uint64_t ogg_granule = 0;

protected:
	void print_opus_error(int error_code) {
		switch (error_code) {
			case OPUS_OK:
				print_line("OpusCodec::OPUS_OK");
				break;
			case OPUS_BAD_ARG:
				print_line("OpusCodec::OPUS_BAD_ARG");
				break;
			case OPUS_BUFFER_TOO_SMALL:
				print_line("OpusCodec::OPUS_BUFFER_TOO_SMALL");
				break;
			case OPUS_INTERNAL_ERROR:
				print_line("OpusCodec::OPUS_INTERNAL_ERROR");
				break;
			case OPUS_INVALID_PACKET:
				print_line("OpusCodec::OPUS_INVALID_PACKET");
				break;
			case OPUS_UNIMPLEMENTED:
				print_line("OpusCodec::OPUS_UNIMPLEMENTED");
				break;
			case OPUS_INVALID_STATE:
				print_line("OpusCodec::OPUS_INVALID_STATE");
				break;
			case OPUS_ALLOC_FAIL:
				print_line("OpusCodec::OPUS_ALLOC_FAIL");
				break;
		}
	}

public:
	Ref<SpeechDecoder> get_speech_decoder() {
		int error;
		::OpusDecoder *decoder = opus_decoder_create(SAMPLE_RATE, CHANNEL_COUNT, &error);
		if (error != OPUS_OK) {
			ERR_PRINT("OpusCodec: could not create Opus decoder!");
			return NULL;
		}

#if SPEECH_DECODER_POLYMORPHISM
		Ref<OpusSpeechDecoder> speech_decoder = OpusSpeechDecoder::_new();
#else
		Ref<SpeechDecoder> speech_decoder = memnew(SpeechDecoder);
#endif
		speech_decoder->set_decoder(decoder);

		return speech_decoder;
	}

	int encode_buffer(const PackedByteArray *p_pcm_buffer, PackedByteArray *p_output_buffer) {
		int number_of_bytes = -1;

		// The following line disables compression and sends data uncompressed.
		// Combine it with a change in speech_decoder.h
		/*memcpy(p_output_buffer->ptrw(), p_pcm_buffer->ptr() + 1, BUFFER_FRAME_COUNT * 2 - 1);
		return BUFFER_FRAME_COUNT * 2 - 1;*/

		if (encoder) {
			const opus_int16 *pcm_buffer_pointer = reinterpret_cast<const opus_int16 *>(p_pcm_buffer->ptr());

			opus_int32 ret_value = opus_encode(encoder, pcm_buffer_pointer, BUFFER_FRAME_COUNT, internal_buffer, INTERNAL_BUFFER_SIZE);
			if (ret_value >= 0) {
				number_of_bytes = ret_value;

				if (number_of_bytes > 0) {
					unsigned char *output_buffer_pointer = reinterpret_cast<unsigned char *>(p_output_buffer->ptrw());
					memcpy(output_buffer_pointer, internal_buffer, number_of_bytes);
				}
			} else {
				print_opus_error(ret_value);
			}
		}
		unsigned char *packet = oggp_get_packet_buffer(oggp, number_of_bytes);
		//nbBytes = opeint_encode_float(&enc->st, &enc->buffer[enc->channels * enc->buffer_start],
		//		enc->buffer_end - enc->buffer_start, packet, max_packet_size);
		memcpy(packet, internal_buffer, number_of_bytes);
		ogg_granule += BUFFER_FRAME_COUNT;
		oggp_commit_packet(oggp, number_of_bytes, ogg_granule, 0);
		oggp_flush_page(oggp);
		unsigned char *page;
		int len;
		while (oggp_get_next_page(oggp, &page, &len)) {
			fwrite(page, len, 1, ogg_output_file);
		}
		fflush(ogg_output_file);
		if (0 && xxdecoder) {
			opus_int16 *output_buffer_pointer = reinterpret_cast<opus_int16 *>(p_output_buffer->ptrw());
			memset(output_buffer_pointer, 0, BUFFER_FRAME_COUNT * 2);
			const unsigned char *opus_buffer_pointer = reinterpret_cast<const unsigned char *>(internal_buffer);

			opus_int32 ret_value = opus_decode(xxdecoder, opus_buffer_pointer, number_of_bytes, output_buffer_pointer, BUFFER_FRAME_COUNT, 0);
			memmove(output_buffer_pointer, ((char *)output_buffer_pointer) + 1, BUFFER_FRAME_COUNT * 2 - 1);
			return BUFFER_FRAME_COUNT * 2 - 1;
		}

		return number_of_bytes;
	}

	bool decode_buffer(
			SpeechDecoder *p_speech_decoder,
			const PackedByteArray *p_compressed_buffer,
			PackedByteArray *p_pcm_output_buffer,
			const int p_compressed_buffer_size,
			const int p_pcm_output_buffer_size) {
		if (p_pcm_output_buffer->size() != p_pcm_output_buffer_size) {
			ERR_PRINT("OpusCodec: decode_buffer output_buffer_size mismatch!");
			return false;
		}

		return p_speech_decoder->process(p_compressed_buffer, p_pcm_output_buffer, p_compressed_buffer_size, p_pcm_output_buffer_size, BUFFER_FRAME_COUNT);
	}

//	int opeint_opus_header_to_packet() {
		/* Version is 1 */
		//ch = 1;
		//if (!write_chars(&p, &ch, 1))
		//	return 0;

		//ch = h->channels;
		//if (!write_chars(&p, &ch, 1))
		//	return 0;

		//if (!write_uint16(&p, 0))//h->preskip))
		//	return 0;

		//if (!write_uint32(&p, h->input_sample_rate))
		//	return 0;

		//if (!write_uint16(&p, h->gain))
		//	return 0;

		//ch = h->channel_mapping;
		//if (!write_chars(&p, &ch, 1))
		//	return 0;
	//}

	OpusCodec() {
		print_line("OpusCodec::OpusCodec");
		int error = 0;
		encoder = opus_encoder_create(SAMPLE_RATE, CHANNEL_COUNT, APPLICATION, &error);

		if (error != OPUS_OK) {
			ERR_PRINT("OpusCodec: could not create Opus encoder!");
		}
		// allowed half-sample-rate.
		error = opus_encoder_ctl(encoder, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND)); //OPUS_BANDWIDTH_NARROWBAND)); //OPUS_AUTO));
		if (error != OPUS_OK) {
			print_opus_error(error);
		}
		error = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(128000)); //8000));
		if (error != OPUS_OK) {
			print_opus_error(error);
		}
		xxdecoder = opus_decoder_create(SAMPLE_RATE, CHANNEL_COUNT, &error);
		if (error != OPUS_OK) {
			ERR_PRINT("OpusCodec: could not create Opus decoder!");
		}
		char x[100];
		sprintf(x, "output%d.ogg", ((int)(rand() + ((intptr_t)this))) % 9887);
		ogg_output_file = fopen(x, "wb");
		oggp = oggp_create(0);
		oggp_set_muxing_delay(oggp, 384);
		/* 19 bytes from fixed header,
		 * 2 bytes for nb_streams & nb_coupled,
		 * 1 byte per channel
		 */
		int header_size = 19;
		unsigned char *packet = oggp_get_packet_buffer(oggp, header_size);
		memcpy(packet, (const unsigned char *)"OpusHead\x01\x01\x00\x00\x80\xbb\x00\x00\x00\x00\x00\x00\x00", header_size); // \x01\x00\x00
		packet[12] = (unsigned char)(SAMPLE_RATE & 255);
		packet[13] = (unsigned char)((SAMPLE_RATE >> 8) & 255);
		packet[14] = (unsigned char)((SAMPLE_RATE >> 16) & 255);
		oggp_commit_packet(oggp, header_size, 0, 0);
		oggp_flush_page(oggp);
	}

	~OpusCodec() {
		print_line("OpusCodec::~OpusCodec");
		opus_encoder_destroy(encoder);
		opus_decoder_destroy(xxdecoder);
		int number_of_bytes = 0;
		unsigned char *packet = oggp_get_packet_buffer(oggp, number_of_bytes);
		*packet = 0;
		ogg_granule += BUFFER_FRAME_COUNT;
		oggp_commit_packet(oggp, number_of_bytes, ogg_granule, 1);
		oggp_flush_page(oggp);
		unsigned char *page;
		int len;
		while (oggp_get_next_page(oggp, &page, &len)) {
			fwrite(page, len, 1, ogg_output_file);
		}
		oggp_destroy(oggp);
		fflush(ogg_output_file);
		fclose(ogg_output_file);
	}
};

#endif // OPUS_CODEC_HPP
