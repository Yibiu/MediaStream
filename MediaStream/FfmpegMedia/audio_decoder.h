#pragma once
#pragma warning(disable:4996)
#include <stdlib.h>
#include <stdint.h>
extern "C"
{
#include "libavutil/frame.h"
#include "libavutil/mem.h"
#include "libavcodec/avcodec.h"
}
#include "../Common/rt_status.h"
#include "../Common/media_defs.h"


/**
* @brief:
* Audio decode callback
*/
class ICAudioDecoderCallback
{
public:
	virtual void on_audio_decoder_callback(uint32_t size, const uint8_t *data_ptr) = 0;
};


/**
* @brief:
* Audio decode class using ffmepg decoding
*
* Only support AAC now
*/
class CAudioDecoder
{
public:
	CAudioDecoder();
	virtual ~CAudioDecoder();

	rt_status_t create(ICAudioDecoderCallback *callback_ptr);
	void destroy();
	rt_status_t send_data(uint32_t size, const uint8_t *data_ptr);

protected:
	rt_status_t _do_decode(AVPacket *packet_ptr);
	bool _resize_buffer(uint32_t new_size);

protected:
	ICAudioDecoderCallback *_callback_ptr;

	AVCodec *_codec_ptr;
	AVCodecContext *_codec_context_ptr;
	AVCodecParserContext *_codec_parser_ptr;
	AVPacket *_packet_ptr;
	AVFrame *_frame_ptr;

	uint32_t _buffer_size;
	uint8_t *_buffer_ptr;
};

