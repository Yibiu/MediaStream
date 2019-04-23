#include "audio_decoder.h"


CAudioDecoder::CAudioDecoder()
{
	_callback_ptr = NULL;

	_codec_ptr = NULL;
	_codec_context_ptr = NULL;
	_codec_parser_ptr = NULL;
	_packet_ptr = NULL;
	_frame_ptr = NULL;

	_buffer_size = 0;
	_buffer_ptr = NULL;
}

CAudioDecoder::~CAudioDecoder()
{
}

rt_status_t CAudioDecoder::create(ICAudioDecoderCallback *callback_ptr)
{
	rt_status_t status = RT_STATUS_SUCCESS;

	do {
		_codec_ptr = avcodec_find_decoder(AV_CODEC_ID_AAC);
		if (NULL == _codec_ptr) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		_codec_parser_ptr = av_parser_init(_codec_ptr->id);
		if (NULL == _codec_parser_ptr) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		_codec_context_ptr = avcodec_alloc_context3(_codec_ptr);
		if (NULL == _codec_context_ptr) {
			status = RT_STATUS_INITIALIZED;
			break;
		}
		if (avcodec_open2(_codec_context_ptr, _codec_ptr, NULL) < 0) {
			status = RT_STATUS_INITIALIZED;
			break;
		}

		_packet_ptr = av_packet_alloc();
		if (NULL == _packet_ptr) {
			status = RT_STATUS_MEMORY_ALLOCATE;
			break;
		}
		_frame_ptr = av_frame_alloc();
		if (NULL == _frame_ptr) {
			status = RT_STATUS_MEMORY_ALLOCATE;
			break;
		}

		_callback_ptr = callback_ptr;
	} while (false);

	if (!rt_is_success(status)) {
		destroy();
	}

	return status;
}

void CAudioDecoder::destroy()
{
	if (NULL != _codec_context_ptr) {
		// Flush...
		//_packet_ptr->size = 0;
		//_packet_ptr->data = NULL;
		//_do_decode(_packet_ptr);
		avcodec_close(_codec_context_ptr);
		avcodec_free_context(&_codec_context_ptr);
		_codec_context_ptr = NULL;
	}
	if (NULL != _codec_parser_ptr) {
		av_parser_close(_codec_parser_ptr);
		_codec_parser_ptr = NULL;
	}
	if (NULL != _packet_ptr) {
		av_packet_free(&_packet_ptr);
		_packet_ptr = NULL;
	}
	if (NULL != _frame_ptr) {
		av_frame_free(&_frame_ptr);
		_frame_ptr = NULL;
	}
	_codec_ptr = NULL;

	if (NULL == _buffer_ptr) {
		delete[] _buffer_ptr;
		_buffer_ptr = NULL;
	}
	_buffer_size = 0;
}

rt_status_t CAudioDecoder::send_data(uint32_t size, const uint8_t *data_ptr)
{
	rt_status_t status = RT_STATUS_SUCCESS;

	do {
		if (size > 0 && NULL == data_ptr) {
			status = RT_STATUS_INVALID_PARAMETER;
			break;
		}

		int32_t in_size = size;
		uint8_t *in_data_ptr = (uint8_t *)data_ptr;
		while (in_size > 0)
		{
			int ret = av_parser_parse2(_codec_parser_ptr, _codec_context_ptr, &_packet_ptr->data,
				&_packet_ptr->size, in_data_ptr, in_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (ret < 0) {
				status = RT_STATUS_EXECUTE;
				break;
			}
			in_data_ptr += ret;
			in_size -= ret;

			if (_packet_ptr->size > 0) {
				status = _do_decode(_packet_ptr);
			}
		}
	} while (false);

	return status;
}

rt_status_t CAudioDecoder::_do_decode(AVPacket *packet_ptr)
{
	rt_status_t status = RT_STATUS_SUCCESS;

	do {
		if (avcodec_send_packet(_codec_context_ptr, packet_ptr) < 0) {
			status = RT_STATUS_EXECUTE;
			break;
		}

		while (true)
		{
			int ret = avcodec_receive_frame(_codec_context_ptr, _frame_ptr);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			}
			else if (ret < 0) {
				status = RT_STATUS_EXECUTE;
				break;
			}
			else {
				if (NULL != _callback_ptr) {
					// FLTP
					if (AV_SAMPLE_FMT_FLTP == _codec_context_ptr->sample_fmt) {
						uint32_t sample_size = av_get_bytes_per_sample(_codec_context_ptr->sample_fmt);
						uint32_t total_size = sample_size * _frame_ptr->nb_samples * _codec_context_ptr->channels;
						if (_buffer_size < total_size) {
							if (!_resize_buffer(total_size)) {
								status = RT_STATUS_MEMORY_ALLOCATE;
								break;
							}
						}

						memcpy(_buffer_ptr, _frame_ptr->data[0], sample_size * _frame_ptr->nb_samples);
						for (int32_t i = 0; i < _frame_ptr->nb_samples; i++) {
							memcpy(_buffer_ptr + 2 * i * sample_size, _frame_ptr->data[0] + i * sample_size, sample_size);
							memcpy(_buffer_ptr + 2 * i * sample_size + sample_size, _frame_ptr->data[1] + i * sample_size, sample_size);
						}
						_callback_ptr->on_audio_decoder_callback(total_size, _buffer_ptr);
					}
					// Other formats
					else {
						// TODO:
						// ...
					}
				}
			}
		}
	} while (false);

	return status;
}

bool CAudioDecoder::_resize_buffer(uint32_t new_size)
{
	if (NULL != _buffer_ptr) {
		delete[] _buffer_ptr;
		_buffer_ptr = NULL;
	}

	_buffer_ptr = new uint8_t[new_size];
	if (NULL == _buffer_ptr) {
		_buffer_size = 0;
		return false;
	}

	_buffer_size = new_size;
	return true;
}

