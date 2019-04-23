#pragma once
#pragma warning(disable:4996)
#include <stdlib.h>
#include <stdint.h>
#include <string>
extern "C"
{
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"
#include "libavutil/intreadwrite.h"
#include "libavformat/avformat.h"
}
#include "../Common/rt_status.h"
#include "../Common/media_defs.h"
#include "../Common/threadtool.h"


/**
* @brief:
* Demux callbck
*/
class ICDemuxCallback
{
public:
	virtual void on_demux_video_data(uint32_t size, const uint8_t *data_ptr, const frame_info_t &frm) = 0;
	virtual void on_demux_audio_data(uint32_t size, const uint8_t *data_ptr, const frame_info_t &frm) = 0;
};


/**
* @brief:
* Demux class using ffmpeg for media file demuxing
*
* Audio: The audio needs ADTS header before every frame.(AAC Frame = ADTS + data)
*/
class CDemuxing : public CThread
{
public:
	CDemuxing();
	virtual ~CDemuxing();

	rt_status_t create(const char *path_ptr, ICDemuxCallback *callback_ptr);
	void destroy();
	rt_status_t get_video_info(video_enc_config_t &cfg, video_enc_param_t &params);
	rt_status_t get_audio_info(audio_enc_config_t &cfg, audio_enc_param_t &params);

	// CThread
	bool start();
	void stop();
	virtual void run();

protected:
	video_fourcc_t _get_video_fourcc(AVPixelFormat fmt);
	audio_fourcc_t _get_audio_fourcc(AVSampleFormat fmt);
	bool _parse_h264_extradata(video_enc_param_t &params);
	bool _parse_hevc_extradata(video_enc_param_t &params);
	bool _parse_aac_extradata(audio_enc_param_t &params);

protected:
	ICDemuxCallback *_callback_ptr;
	bool _running;

	AVFormatContext *_context_ptr;
	AVStream *_vstream_ptr;
	AVStream *_astream_ptr;
	video_enc_param_t _video_params;
	audio_enc_param_t _audio_params;

	uint32_t _adts_buf_size;
	uint8_t *_adts_buf_ptr;
};



