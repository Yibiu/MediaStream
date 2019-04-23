#include "demuxing.h"


CDemuxing::CDemuxing()
{
	_callback_ptr = NULL;
	_running = false;

	_context_ptr = NULL;
	_vstream_ptr = NULL;
	_astream_ptr = NULL;

	_adts_buf_size = 0;
	_adts_buf_ptr = NULL;
}

CDemuxing::~CDemuxing()
{
}

rt_status_t CDemuxing::create(const char *path_ptr, ICDemuxCallback *callback_ptr)
{
	rt_status_t status = RT_STATUS_SUCCESS;

	do {
		if (NULL == path_ptr) {
			status = RT_STATUS_INVALID_PARAMETER;
			break;
		}

		if (avformat_open_input(&_context_ptr, path_ptr, NULL, NULL) < 0) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		if (avformat_find_stream_info(_context_ptr, NULL) < 0) {
			status = RT_STATUS_INITIALIZED;
			break;
		}

		int index = av_find_best_stream(_context_ptr, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
		if (index < 0) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		_vstream_ptr = _context_ptr->streams[index];
		index = av_find_best_stream(_context_ptr, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
		if (index >= 0) {
			_astream_ptr = _context_ptr->streams[index];

			_adts_buf_size = _astream_ptr->codecpar->frame_size * _astream_ptr->codecpar->channels * 2; // 2 is reserved
			_adts_buf_ptr = new uint8_t[_adts_buf_size];
			if (NULL == _adts_buf_ptr) {
				status = RT_STATUS_MEMORY_ALLOCATE;
				break;
			}
		}

		//av_dump_format(_context_ptr, 0, path_ptr, 0);
		_callback_ptr = callback_ptr;
	} while (false);

	if (!rt_is_success(status)) {
		destroy();
	}

	return status;
}

void CDemuxing::destroy()
{
	if (NULL != _context_ptr) {
		avformat_free_context(_context_ptr);
		_context_ptr = NULL;
	}
	if (NULL != _adts_buf_ptr) {
		delete[] _adts_buf_ptr;
		_adts_buf_ptr = NULL;
	}
	_adts_buf_size = 0;
}

rt_status_t CDemuxing::get_video_info(video_enc_config_t &cfg, video_enc_param_t &params)
{
	rt_status_t status = RT_STATUS_SUCCESS;

	do {
		if (NULL == _context_ptr || NULL == _vstream_ptr) {
			status = RT_STATUS_UNINITIALIZED;
			break;
		}

		switch (_vstream_ptr->codec->codec_id)
		{
		case AV_CODEC_ID_H264:
			cfg.codec = VIDEO_CODEC_H264;
			if (!_parse_h264_extradata(params)) {
				status = RT_STATUS_UNSUPPORT;
			}
			break;
		case AV_CODEC_ID_HEVC:
			cfg.codec = VIDEO_CODEC_HEVC;
			if (!_parse_hevc_extradata(params)) {
				status = RT_STATUS_UNSUPPORT;
			}
			break;
		default:
			status = RT_STATUS_UNSUPPORT;
			break;
		}
		if (!rt_is_success(status))
			break;
		cfg.preset = VIDEO_PRESET_DEFAULT;
		cfg.profile = VIDEO_PROFILE_DEFAULT;
		cfg.fourcc = _get_video_fourcc(_vstream_ptr->codec->pix_fmt);
		cfg.width = _vstream_ptr->codec->width;
		cfg.height = _vstream_ptr->codec->height;
		cfg.fps_num = _vstream_ptr->codec->framerate.num;
		cfg.fps_den = _vstream_ptr->codec->framerate.den;
		cfg.bitrate = _vstream_ptr->codec->bit_rate;
		cfg.gop = _vstream_ptr->codec->gop_size;
		cfg.b_frames = _vstream_ptr->codec->max_b_frames;

		_video_params = params;
	} while (false);

	return status;
}

rt_status_t CDemuxing::get_audio_info(audio_enc_config_t &cfg, audio_enc_param_t &params)
{
	rt_status_t status = RT_STATUS_SUCCESS;

	do {
		if (NULL == _context_ptr || NULL == _vstream_ptr) {
			status = RT_STATUS_UNINITIALIZED;
			break;
		}
		if (NULL == _astream_ptr) {
			status = RT_STATUS_UNSUPPORT;
			break;
		}

		switch (_astream_ptr->codec->codec_id)
		{
		case AV_CODEC_ID_AAC:
			if (!_parse_aac_extradata(params)) {
				status = RT_STATUS_UNSUPPORT;
			}
			break;
		default:
			status = RT_STATUS_UNSUPPORT;
			break;
		}
		if (!rt_is_success(status))
			break;
		cfg.bitrate = _astream_ptr->codec->bit_rate;
		cfg.bitdepth = 16;
		cfg.fourcc = _get_audio_fourcc(_astream_ptr->codec->sample_fmt);
		cfg.channels = _astream_ptr->codec->channels;
		cfg.samplerate = _astream_ptr->codec->sample_rate;
		cfg.samples_per_frame = _astream_ptr->codec->frame_size;

		_audio_params = params;
	} while (false);

	return status;
}

bool CDemuxing::start()
{
	_running = true;
	return CThread::start();
}

void CDemuxing::stop()
{
	_running = false;
	CThread::stop();
}

void CDemuxing::run()
{
	AVPacket packet;
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;
	while (_running && (av_read_frame(_context_ptr, &packet) >= 0))
	{
		if (packet.stream_index == _vstream_ptr->index) {
			switch (_vstream_ptr->codec->codec_id)
			{
			case AV_CODEC_ID_H264:
			{
				// Replace 4 bytes of size with start code: 0x00, 0x00, 0x00, 0x01
				packet.data[0] = 0x00;
				packet.data[1] = 0x00;
				packet.data[2] = 0x00;
				packet.data[3] = 0x01;

				if (NULL != _callback_ptr) {
					frame_info_t frm_info;
					frm_info.is_video = true;
					frm_info.key_frame = packet.flags & AV_PKT_FLAG_KEY;
					frm_info.pts = packet.pts;
					frm_info.dts = packet.dts;
					_callback_ptr->on_demux_video_data(packet.size, packet.data, frm_info);
				}
			}
			break;
			case AV_CODEC_ID_HEVC:
			{
				// Replace 4 bytes of size with start code: 0x00, 0x00, 0x00, 0x01
				// Key frame prefix with VPS/SPS/PPS/SEI_PREFIX
				// We assume that the prefix nuals here are same with the nuals in extra data!!!
				if (packet.flags & AV_PKT_FLAG_KEY) {
					uint32_t size_xps = _video_params.size_vps + _video_params.size_sps + _video_params.size_pps + 12;
					uint32_t size_sei_prefix = packet.data[size_xps++] << 24;
					size_sei_prefix |= packet.data[size_xps++] << 16;
					size_sei_prefix |= packet.data[size_xps++] << 8;
					size_sei_prefix |= packet.data[size_xps++];
					size_xps += size_sei_prefix;

					packet.data[size_xps] = 0x00;
					packet.data[size_xps + 1] = 0x00;
					packet.data[size_xps + 2] = 0x00;
					packet.data[size_xps + 3] = 0x01;

					if (NULL != _callback_ptr) {
						frame_info_t frm_info;
						frm_info.is_video = true;
						frm_info.key_frame = true;
						frm_info.pts = packet.pts;
						frm_info.dts = packet.dts;
						_callback_ptr->on_demux_video_data(packet.size - size_xps, packet.data + size_xps, frm_info);
					}
				}
				else {
					packet.data[0] = 0x00;
					packet.data[1] = 0x00;
					packet.data[2] = 0x00;
					packet.data[3] = 0x01;

					if (NULL != _callback_ptr) {
						frame_info_t frm_info;
						frm_info.is_video = true;
						frm_info.key_frame = false;
						frm_info.pts = packet.pts;
						frm_info.dts = packet.dts;
						_callback_ptr->on_demux_video_data(packet.size, packet.data, frm_info);
					}
				}
			}
			break;
			default:
				break;
			}
		}
		else if (NULL != _astream_ptr && packet.stream_index == _astream_ptr->index) {
			switch (_astream_ptr->codec->codec_id)
			{
			case AV_CODEC_ID_AAC:
			{
				// Add 7 bytes of ADTS header to each frame, while some file does not need when to play
				uint32_t sample_index = ((_audio_params.data_esds[0] & 0x07) << 1) | (_audio_params.data_esds[1] >> 7);
				uint32_t channels = ((_audio_params.data_esds[1]) & 0x7f) >> 3;

				uint32_t size = packet.size + 7;
				_adts_buf_ptr[0] = 0xff;
				_adts_buf_ptr[1] = 0xf1;
				_adts_buf_ptr[2] = 0x40 | (sample_index << 2) | (channels >> 2);
				_adts_buf_ptr[3] = ((channels & 0x3) << 6) | (size >> 11);
				_adts_buf_ptr[4] = (size >> 3) & 0xff;
				_adts_buf_ptr[5] = ((size << 5) & 0xff) | 0x1f;
				_adts_buf_ptr[6] = 0xfc;

				if (NULL != _callback_ptr) {
					frame_info_t frm_info;
					frm_info.is_video = false;
					frm_info.key_frame = false;
					frm_info.pts = packet.pts;
					frm_info.dts = packet.dts;
					memcpy(_adts_buf_ptr + 7, packet.data, packet.size);
					_callback_ptr->on_demux_audio_data(size, _adts_buf_ptr, frm_info);
				}
			}
			break;
			default:
				break;
			}
		}
	}
}


video_fourcc_t CDemuxing::_get_video_fourcc(AVPixelFormat fmt)
{
	video_fourcc_t fourcc = VIDEO_DOURCC_UNKNOWN;
	switch (fmt)
	{
	case AV_PIX_FMT_RGBA:
		fourcc = VIDEO_FOURCC_RGBA;
	case AV_PIX_FMT_BGRA:
		fourcc = VIDEO_FOURCC_BGRA;
		break;
	case AV_PIX_FMT_ARGB:
		fourcc = VIDEO_FOURCC_ARGB;
		break;
	case AV_PIX_FMT_ABGR:
		fourcc = VIDEO_FOURCC_ABGR;
		break;
	case AV_PIX_FMT_RGB24:
		fourcc = VIDEO_FOURCC_RGB;
		break;
	case AV_PIX_FMT_BGR24:
		fourcc = VIDEO_FOURCC_BGR;
		break;
	case AV_PIX_FMT_YUV420P:
		fourcc = VIDEO_FOURCC_YUV420;
		break;
	case AV_PIX_FMT_NV12:
		fourcc = VIDEO_FOURCC_NV12;
		break;
	default:
		break;
	}

	return fourcc;
}

audio_fourcc_t CDemuxing::_get_audio_fourcc(AVSampleFormat fmt)
{
	audio_fourcc_t fourcc = AUDIO_FOURCC_UNKNOWN;
	switch (fmt)
	{
	case AV_SAMPLE_FMT_U8:
		fourcc = AUDIO_FOURCC_U8;
		break;
	case AV_SAMPLE_FMT_S16:
		fourcc = AUDIO_FOURCC_S16;
		break;
	case AV_SAMPLE_FMT_S32:
		fourcc = AUDIO_FOURCC_S32;
		break;
	case AV_SAMPLE_FMT_U8P:
		fourcc = AUDIO_FOURCC_U8P;
		break;
	case AV_SAMPLE_FMT_S16P:
		fourcc = AUDIO_FOURCC_S16P;
		break;
	case AV_SAMPLE_FMT_S32P:
		fourcc = AUDIO_FOURCC_S32P;
		break;
	case AV_SAMPLE_FMT_FLT:
		fourcc = AUDIO_FOURCC_FLT;
		break;
	case AV_SAMPLE_FMT_FLTP:
		fourcc = AUDIO_FOURCC_FLTP;
		break;
	default:
		break;
	}

	return fourcc;
}

bool CDemuxing::_parse_h264_extradata(video_enc_param_t &params)
{
	if (_vstream_ptr->codec->extradata_size < 5)
		return false;

	uint32_t offset = 0;
	uint8_t *data_ptr = _vstream_ptr->codec->extradata + 5;
	// AVCDecoderConfiguration
	// SPS(Only save the first)
	uint32_t num_sps = data_ptr[offset++] & 0x1f;
	for (uint32_t i = 0; i < num_sps; i++) {
		uint32_t size_sps = (data_ptr[offset++] << 8);
		size_sps |= data_ptr[offset++];

		if (0 == i) {
			params.size_sps = size_sps;
			memcpy(params.data_sps, data_ptr + offset, size_sps);
		}
		offset += size_sps;
	}
	// PPS(Only save the first)
	uint32_t num_pps = data_ptr[offset++];
	for (uint32_t i = 0; i < num_pps; i++) {
		uint32_t size_pps = (data_ptr[offset++] << 8);
		size_pps |= data_ptr[offset++];

		if (i == 0) {
			params.size_pps = size_pps;
			memcpy(params.data_pps, data_ptr + offset, size_pps);
		}
		offset += size_pps;
	}

	return (0 != params.size_sps && 0 != params.size_pps);
}

bool CDemuxing::_parse_hevc_extradata(video_enc_param_t &params)
{
	if (_vstream_ptr->codec->extradata_size < 22)
		return false;

	uint32_t offset = 0;
	uint8_t *data_ptr = _vstream_ptr->codec->extradata + 22;
	// HEVCDecoderConfiguration
	uint32_t num_arrays = data_ptr[offset++];
	bool error = false;
	for (uint32_t i = 0; i < num_arrays; i++) {
		switch ((data_ptr[offset++] & 0x3f))
		{
		case NAL_TYPE_VPS:
		{
			uint32_t num_vps = (data_ptr[offset++] << 8);
			num_vps |= data_ptr[offset++];
			for (uint32_t j = 0; j < num_vps; j++) {
				uint32_t size_vps = (data_ptr[offset++] << 8);
				size_vps |= data_ptr[offset++];

				if (j == 0) {
					params.size_vps = size_vps;
					memcpy(params.data_vps, data_ptr + offset, size_vps);
				}
				offset += size_vps;
			}
		}
		break;
		case NAL_TYPE_SPS:
		{
			uint32_t num_sps = (data_ptr[offset++] << 8);
			num_sps |= data_ptr[offset++];
			for (uint32_t j = 0; j < num_sps; j++) {
				uint32_t size_sps = (data_ptr[offset++] << 8);
				size_sps |= data_ptr[offset++];

				if (j == 0) {
					params.size_sps = size_sps;
					memcpy(params.data_sps, data_ptr + offset, size_sps);
				}
				offset += size_sps;
			}
		}
		break;
		case NAL_TYPE_PPS:
		{
			uint32_t num_pps = (data_ptr[offset++] << 8);
			num_pps |= data_ptr[offset++];
			for (uint32_t j = 0; j < num_pps; j++) {
				uint32_t size_pps = (data_ptr[offset++] << 8);
				size_pps |= data_ptr[offset++];

				if (j == 0) {
					params.size_pps = size_pps;
					memcpy(params.data_pps, data_ptr + offset, size_pps);
				}
				offset += size_pps;
			}
		}
		break;
		case NAL_TYPE_SEI_PREFIX:
		{
			uint32_t num_sei_prefix = (data_ptr[offset++] << 8);
			num_sei_prefix |= data_ptr[offset++];
			for (uint32_t j = 0; j < num_sei_prefix; j++) {
				uint32_t size_sei_prefix = (data_ptr[offset++] << 8);
				size_sei_prefix |= data_ptr[offset++];

				if (j == 0) {
					params.size_sei_prefix = size_sei_prefix;
					memcpy(params.data_sei_prefix, data_ptr + offset, size_sei_prefix);
				}
				offset += size_sei_prefix;
			}
		}
		break;
		case NAL_TYPE_SEI_SUFFIX:
		{
			uint32_t num_sei_suffix = (data_ptr[offset++] << 8);
			num_sei_suffix |= data_ptr[offset++];
			for (uint32_t j = 0; j < num_sei_suffix; j++) {
				uint32_t size_sei_suffix = (data_ptr[offset++] << 8);
				size_sei_suffix |= data_ptr[offset++];

				if (j == 0) {
					params.size_sei_suffix = size_sei_suffix;
					memcpy(params.data_sei_suffix, data_ptr + offset, size_sei_suffix);
				}
				offset += size_sei_suffix;
			}
		}
		break;
		default:
			error = true;
			break;
		}
		if (error)
			break;
	}

	return (0 != params.size_vps && 0 != params.size_sps && 0 != params.size_pps);
}

bool CDemuxing::_parse_aac_extradata(audio_enc_param_t &params)
{
	if (_astream_ptr->codec->extradata_size < 2)
		return false;

	// AudioSpecificConfig
	params.size_esds = _astream_ptr->codec->extradata_size;
	memcpy(params.data_esds, _astream_ptr->codec->extradata, params.size_esds);
	return true;
}


