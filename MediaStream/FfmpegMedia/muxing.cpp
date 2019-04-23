#include "muxing.h"


CMuxing::CMuxing()
{
	_context_ptr = NULL;
	_vstream_ptr = NULL;
	_astream_ptr = NULL;
	_metadata_ptr = NULL;

	_fps_num = 0;
	_fps_den = 0;
	_video_frame_count = 0;
	_audio_frame_count = 0;
}

CMuxing::~CMuxing()
{
}

rt_status_t CMuxing::create(const char *path_ptr,
	const video_enc_config_t &vcfg, const video_enc_param_t &vparams,
	const audio_enc_config_t &acfg, const audio_enc_param_t &aparams)
{
	rt_status_t status = RT_STATUS_SUCCESS;

	do {
		if (NULL == path_ptr) {
			status = RT_STATUS_INVALID_PARAMETER;
			break;
		}

		if (avformat_alloc_output_context2(&_context_ptr, NULL, NULL, path_ptr) < 0) {
			if (avformat_alloc_output_context2(&_context_ptr, NULL, "mpeg", path_ptr) < 0) {
				status = RT_STATUS_INITIALIZED;
				break;
			}
		}
		_vstream_ptr = avformat_new_stream(_context_ptr, NULL);
		if (NULL == _vstream_ptr) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		_vstream_ptr->id = _context_ptr->nb_streams - 1;
		_astream_ptr = avformat_new_stream(_context_ptr, NULL);
		if (NULL == _astream_ptr) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		_astream_ptr->id = _context_ptr->nb_streams - 1;

		// Video stream
		uint32_t extra_size = 0;
		uint8_t extra_data[2048];
		if (!_generate_h264_extradata(vparams, extra_size, extra_data)) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		_vstream_ptr->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
		_vstream_ptr->codecpar->codec_id = AV_CODEC_ID_H264;
		_vstream_ptr->codecpar->codec_tag = 0;
		_vstream_ptr->codecpar->width = vcfg.width;
		_vstream_ptr->codecpar->height = vcfg.height;
		_vstream_ptr->codecpar->format = _get_video_format(vcfg.fourcc);
		_vstream_ptr->codecpar->bit_rate = vcfg.bitrate;
		_vstream_ptr->codecpar->extradata_size = extra_size;
		_vstream_ptr->codecpar->extradata = (uint8_t *)av_malloc(_vstream_ptr->codecpar->extradata_size
			+ AV_INPUT_BUFFER_PADDING_SIZE); // Align
		memcpy(_vstream_ptr->codecpar->extradata, extra_data, _vstream_ptr->codecpar->extradata_size);
		_fps_num = vcfg.fps_num;
		_fps_den = vcfg.fps_den;

		// Auido stream
		if (!_generate_aac_extradata(aparams, extra_size, extra_data)) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		_astream_ptr->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
		_astream_ptr->codecpar->codec_id = AV_CODEC_ID_AAC;
		_astream_ptr->codecpar->codec_tag = 0;
		_astream_ptr->codecpar->format = _get_audio_format(acfg.fourcc);
		_astream_ptr->codecpar->bit_rate = acfg.bitrate;
		_astream_ptr->codecpar->channel_layout = AV_CH_LAYOUT_STEREO;
		_astream_ptr->codecpar->channels = acfg.channels;
		_astream_ptr->codecpar->sample_rate = acfg.samplerate;
		_astream_ptr->codecpar->frame_size = acfg.samples_per_frame;
		_astream_ptr->codecpar->extradata_size = extra_size;
		_astream_ptr->codecpar->extradata = (uint8_t *)av_malloc(_astream_ptr->codecpar->extradata_size
			+ AV_INPUT_BUFFER_PADDING_SIZE); // Align
		memcpy(_astream_ptr->codecpar->extradata, extra_data, _astream_ptr->codecpar->extradata_size);

		// Meta data
		_context_ptr->metadata = _metadata_ptr;

		//av_dump_format(_context_ptr, 0, path_ptr, 1);
		if (avio_open(&_context_ptr->pb, path_ptr, AVIO_FLAG_WRITE) < 0) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		if (avformat_write_header(_context_ptr, NULL) < 0) {
			status = RT_STATUS_INITIALIZED;
			break;
		}

		_video_frame_count = 0;
		_audio_frame_count = 0;
	} while (false);

	if (!rt_is_success(status)) {
		destroy();
	}

	return status;
}

void CMuxing::destroy()
{
	if (NULL != _context_ptr) {
		av_write_trailer(_context_ptr);
		avformat_free_context(_context_ptr);
		_context_ptr = NULL;
	}
	_vstream_ptr = NULL;
	_astream_ptr = NULL;

	_fps_num = 0;
	_fps_den = 0;
	_video_frame_count = 0;
	_audio_frame_count = 0;
}

rt_status_t CMuxing::set_metadata(const char *key, const char *value)
{
	rt_status_t status = RT_STATUS_SUCCESS;

	if (av_dict_set(&_metadata_ptr, key, value, 0) < 0) {
		status = RT_STATUS_INVALID_PARAMETER;
	}
	return status;
}

rt_status_t CMuxing::write_video(uint32_t size, const uint8_t *data_ptr, bool key_frame)
{
	rt_status_t status = RT_STATUS_SUCCESS;
	AVRational rational = { _fps_den, _fps_num };

	do {
		if (NULL == _context_ptr || NULL == _vstream_ptr) {
			status = RT_STATUS_UNINITIALIZED;
			break;
		}

		AVPacket packet;
		av_init_packet(&packet);
		memset(&packet, 0x00, sizeof(AVPacket));
		packet.stream_index = _vstream_ptr->index;
		packet.size = size;
		packet.data = (uint8_t *)data_ptr;
		packet.pts = _video_frame_count;
		packet.dts = _video_frame_count;
		if (key_frame) {
			packet.flags |= AV_PKT_FLAG_KEY;
		}

		av_packet_rescale_ts(&packet, rational, _vstream_ptr->time_base);
		//if (av_interleaved_write_frame(_context_ptr, &packet) < 0) {
		if (av_write_frame(_context_ptr, &packet) < 0) {
			status = RT_STATUS_EXECUTE;
			break;
		}

		_video_frame_count++;
	} while (false);

	return status;
}

rt_status_t CMuxing::write_audio(uint32_t size, const uint8_t *data_ptr)
{
	rt_status_t status = RT_STATUS_SUCCESS;
	AVRational rational = { 1, _astream_ptr->codecpar->sample_rate };

	do {
		if (NULL == _context_ptr || NULL == _astream_ptr) {
			status = RT_STATUS_UNINITIALIZED;
			break;
		}

		AVPacket packet;
		av_init_packet(&packet);
		memset(&packet, 0x00, sizeof(AVPacket));
		packet.stream_index = _astream_ptr->index;
		packet.size = size;
		packet.data = (uint8_t *)data_ptr;
		packet.pts = _audio_frame_count;
		packet.dts = _audio_frame_count;

		//av_packet_rescale_ts(&packet, rational, _astream_ptr->time_base);
		//if (av_interleaved_write_frame(_context_ptr, &packet) < 0) {
		if (av_write_frame(_context_ptr, &packet) < 0) {
			status = RT_STATUS_EXECUTE;
			break;
		}

		_audio_frame_count += _astream_ptr->codecpar->frame_size;
	} while (false);

	return status;
}


bool CMuxing::_generate_h264_extradata(const video_enc_param_t &params, uint32_t &size, uint8_t *data_ptr)
{
	if (0 == params.size_pps || 0 == params.size_sps)
		return false;

	// AVCDecoderConfiguration
	size = 0;
	data_ptr[size++] = 0x01;
	data_ptr[size++] = params.data_sps[1];
	data_ptr[size++] = params.data_sps[2];
	data_ptr[size++] = params.data_sps[3];
	data_ptr[size++] = 0xff;
	data_ptr[size++] = 0xe1;
	data_ptr[size++] = (params.size_sps >> 8) & 0xff;
	data_ptr[size++] = params.size_sps & 0xff;
	memcpy(data_ptr + size, params.data_sps, params.size_sps);
	size += params.size_sps;
	data_ptr[size++] = 0x01;
	data_ptr[size++] = (params.size_pps >> 8) & 0xff;
	data_ptr[size++] = params.size_pps & 0x00ff;
	memcpy(data_ptr + size, params.data_pps, params.size_pps);
	size += params.size_pps;

	return true;
}

bool CMuxing::_generate_aac_extradata(const audio_enc_param_t &params, uint32_t &size, uint8_t *data_ptr)
{
	if (0 == params.size_esds)
		return false;

	// AudioSpecificConfig
	size = params.size_esds;
	memcpy(data_ptr, params.data_esds, size);
	return true;
}

AVPixelFormat CMuxing::_get_video_format(video_fourcc_t fourcc)
{
	AVPixelFormat fmt = AV_PIX_FMT_NONE;
	switch (fourcc)
	{
	case VIDEO_FOURCC_RGBA:
		fmt = AV_PIX_FMT_RGBA;
		break;
	case VIDEO_FOURCC_BGRA:
		fmt = AV_PIX_FMT_BGRA;
		break;
	case VIDEO_FOURCC_ARGB:
		fmt = AV_PIX_FMT_ARGB;
		break;
	case VIDEO_FOURCC_ABGR:
		fmt = AV_PIX_FMT_ABGR;
		break;
	case VIDEO_FOURCC_RGB:
		fmt = AV_PIX_FMT_RGB24;
		break;
	case VIDEO_FOURCC_BGR:
		fmt = AV_PIX_FMT_BGR24;
		break;
	case VIDEO_FOURCC_YUV420:
		fmt = AV_PIX_FMT_YUV420P;
		break;
	case VIDEO_FOURCC_NV12:
		fmt = AV_PIX_FMT_NV12;
		break;
	default:
		break;
	}

	return fmt;
}

AVSampleFormat CMuxing::_get_audio_format(audio_fourcc_t fourcc)
{
	AVSampleFormat fmt = AV_SAMPLE_FMT_NONE;
	switch (fourcc)
	{
	case AUDIO_FOURCC_U8:
		fmt = AV_SAMPLE_FMT_U8;
		break;
	case AUDIO_FOURCC_S16:
		fmt = AV_SAMPLE_FMT_S16;
		break;
	case AUDIO_FOURCC_S32:
		fmt = AV_SAMPLE_FMT_S32;
		break;
	case AUDIO_FOURCC_U8P:
		fmt = AV_SAMPLE_FMT_U8P;
		break;
	case AUDIO_FOURCC_S16P:
		fmt = AV_SAMPLE_FMT_S16P;
		break;
	case AUDIO_FOURCC_S32P:
		fmt = AV_SAMPLE_FMT_S32P;
		break;
	case AUDIO_FOURCC_FLT:
		fmt = AV_SAMPLE_FMT_FLT;
		break;
	case AUDIO_FOURCC_FLTP:
		fmt = AV_SAMPLE_FMT_FLTP;
		break;
	default:
		break;
	}

	return fmt;
}

