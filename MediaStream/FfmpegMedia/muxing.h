#pragma once
#pragma warning(disable:4996)
#include <stdlib.h>
#include <stdint.h>
#include <string>
extern "C"
{
#include "libavutil\opt.h"
#include "libavutil\mathematics.h"
#include "libavutil\timestamp.h"
#include "libavutil\channel_layout.h"
#include "libavformat\avformat.h"
#include "libavformat\avio.h"
}
#include "../Common/rt_status.h"
#include "../Common/media_defs.h"


/**
* @breif:
* Mux class using ffmpeg for media file muxing
*
* Pts and dts are calculated in the class.
* Need locker outside this class when call write_video and write_audio in diff threads.
* Video: Only H264
* Audio: Only AAC
*/
class CMuxing
{
public:
	CMuxing();
	virtual ~CMuxing();

	rt_status_t create(const char *path_ptr, 
		const video_enc_config_t &vcfg, const video_enc_param_t &vparams,
		const audio_enc_config_t &acfg, const audio_enc_param_t &aparams);
	void destroy();
	rt_status_t set_metadata(const char *key, const char *value);
	rt_status_t write_video(uint32_t size, const uint8_t *data_ptr, bool key_frame);
	rt_status_t write_audio(uint32_t size, const uint8_t *data_ptr);

protected:
	bool _generate_h264_extradata(const video_enc_param_t &params, uint32_t &size, uint8_t *data_ptr);
	bool _generate_aac_extradata(const audio_enc_param_t &params, uint32_t &size, uint8_t *data_ptr);
	AVPixelFormat _get_video_format(video_fourcc_t fourcc);
	AVSampleFormat _get_audio_format(audio_fourcc_t fourcc);

protected:
	AVFormatContext *_context_ptr;
	AVStream *_vstream_ptr;
	AVStream *_astream_ptr;
	AVDictionary *_metadata_ptr;

	uint32_t _fps_num;
	uint32_t _fps_den;
	uint32_t _video_frame_count;
	uint32_t _audio_frame_count;
};

