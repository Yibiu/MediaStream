#pragma once
#pragma warning(disable:4996)
#include <stdlib.h>
#include <stdint.h>
extern "C"
{
#include "libavcodec/avcodec.h"
}
#include "../Common/rt_status.h"
#include "../Common/media_defs.h"


/**
* @brief:
* Video decode callback
*/
class ICVideoDecoderCallback
{
public:
	virtual void on_video_decoder_callback(uint32_t size, const uint8_t *data_ptr) = 0;
};


/**
* @brief:
* Video decode class using ffmepg decoding
*
* Only support H264 now
*/
class CVideoDecoder
{
public:
	CVideoDecoder();
	virtual ~CVideoDecoder();

	rt_status_t create(ICVideoDecoderCallback *callback_ptr);
	void destroy();
	rt_status_t send_data(uint32_t size, const uint8_t *data_ptr);

protected:
	rt_status_t _do_decode(AVPacket *packet_ptr);
	bool _resize_buffer(uint32_t new_size);

protected:
	ICVideoDecoderCallback *_callback_ptr;

	AVCodec *_codec_ptr;
	AVCodecContext *_codec_context_ptr;
	AVCodecParserContext *_codec_parser_ptr;
	AVPacket *_packet_ptr;
	AVFrame *_frame_ptr;

	uint32_t _buffer_size;
	uint8_t *_buffer_ptr;
};
