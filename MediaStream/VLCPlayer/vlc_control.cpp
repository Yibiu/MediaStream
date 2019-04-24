#include "vlc_control.h"


CVLCControl::CVLCControl()
{
	_mem_callback_ptr = NULL;
	_callback_ptr = NULL;

	_instance_ptr = NULL;
	_player_ptr = NULL;
	_media_ptr = NULL;
	_event_ptr = NULL;
}

CVLCControl::~CVLCControl()
{
}

bool CVLCControl::create(CVLCCallback *callback_ptr)
{
	_instance_ptr = libvlc_new(0, NULL);
	if (NULL == _instance_ptr)
		return false;

	_callback_ptr = callback_ptr;
	return true;
}

void CVLCControl::destroy()
{
	if (NULL != _instance_ptr) {
		libvlc_release(_instance_ptr);
		_instance_ptr = NULL;
	}
}

bool CVLCControl::open_file(const char *path_ptr)
{
	bool success = false;
	do {
		if (NULL == _instance_ptr)
			break;

		_media_ptr = libvlc_media_new_path(_instance_ptr, path_ptr);
		if (NULL == _media_ptr)
			break;
		_player_ptr = libvlc_media_player_new_from_media(_media_ptr);
		if (NULL == _player_ptr)
			break;
		_event_ptr = libvlc_media_player_event_manager(_player_ptr);
		if (NULL == _event_ptr)
			break;
		libvlc_event_attach(_event_ptr, libvlc_MediaPlayerTimeChanged, events_callback, this);
		libvlc_event_attach(_event_ptr, libvlc_MediaPlayerEndReached, events_callback, this);
		libvlc_video_set_callbacks(_player_ptr, lock_callback, unlock_callback, display_callback, this);
		libvlc_video_set_format_callbacks(_player_ptr, setup_callback, clearup_callback);

		success = true;
	} while (false);

	if (!success)
		close_file();

	return success;
}

void CVLCControl::close_file()
{
	if (NULL != _player_ptr) {
		libvlc_media_player_release(_player_ptr);
		_player_ptr = NULL;
	}
	if (NULL != _media_ptr) {
		libvlc_media_release(_media_ptr);
		_media_ptr = NULL;
	}
	_event_ptr = NULL;
}

bool CVLCControl::open_mem(CVLCMemCallback *mem_callback)
{
	bool success = false;
	do {
		if (NULL == _instance_ptr)
			break;

		_media_ptr = libvlc_media_new_callbacks(_instance_ptr, mem_open_callback, 
			mem_read_callback, mem_seek_callback, mem_close_callback, this);
		if (NULL == _media_ptr)
			break;
		_player_ptr = libvlc_media_player_new_from_media(_media_ptr);
		if (NULL == _player_ptr)
			break;
		_event_ptr = libvlc_media_player_event_manager(_player_ptr);
		if (NULL == _event_ptr)
			break;
		libvlc_event_attach(_event_ptr, libvlc_MediaPlayerTimeChanged, events_callback, this);
		libvlc_event_attach(_event_ptr, libvlc_MediaPlayerEndReached, events_callback, this);
		libvlc_video_set_callbacks(_player_ptr, lock_callback, unlock_callback, display_callback, this);
		libvlc_video_set_format_callbacks(_player_ptr, setup_callback, clearup_callback);

		_mem_callback_ptr = mem_callback;
		success = true;
	} while (false);

	if (!success)
		close_mem();

	return success;
}

void CVLCControl::close_mem()
{
	if (NULL != _player_ptr) {
		libvlc_media_player_release(_player_ptr);
		_player_ptr = NULL;
	}
	if (NULL != _media_ptr) {
		libvlc_media_release(_media_ptr);
		_media_ptr = NULL;
	}
	_event_ptr = NULL;
}

bool CVLCControl::start()
{
	if (NULL != _player_ptr) {
		return 0 == libvlc_media_player_play(_player_ptr);
	}

	return false;
}

void CVLCControl::pause()
{
	if (NULL != _player_ptr) {
		libvlc_media_player_pause(_player_ptr);
	}
}

void CVLCControl::stop()
{
	if (NULL != _player_ptr) {
		libvlc_media_player_stop(_player_ptr);
	}
}

int CVLCControl::get_state()
{
	if (NULL != _player_ptr) {
		return libvlc_media_player_get_state(_player_ptr);
	}

	return -1;
}

int CVLCControl::get_length()
{
	if (NULL != _player_ptr) {
		return libvlc_media_player_get_length(_player_ptr);
	}

	return -1;
}

int CVLCControl::get_current()
{
	if (NULL != _player_ptr) {
		return libvlc_media_player_get_time(_player_ptr);
	}

	return -1;
}

void CVLCControl::set_current(int time_ms)
{
	if (NULL != _player_ptr) {
		libvlc_media_player_set_time(_player_ptr, time_ms);
	}
}

int CVLCControl::get_volume()
{
	if (NULL != _player_ptr) {
		return libvlc_audio_get_volume(_player_ptr);
	}

	return -1;
}

void CVLCControl::set_volume(int vol)
{
	if (NULL != _player_ptr) {
		libvlc_audio_set_volume(_player_ptr, vol);
	}
}


void CVLCControl::events_callback_internal(const libvlc_event_t *evt_ptr)
{
	switch (evt_ptr->type)
	{
	case libvlc_MediaPlayerTimeChanged:
		if (NULL != _callback_ptr) {
			_callback_ptr->on_play_time_changed(get_current());
		}
		break;
	case libvlc_MediaPlayerEndReached:
		if (NULL != _callback_ptr) {
			_callback_ptr->on_play_finished();
		}
		break;
	default:
		break;
	}
}

void CVLCControl::lock_callback_internal(void **pic)
{
	if (NULL != _callback_ptr) {
		uint8_t *data_ptr = NULL;
		_callback_ptr->on_lock_buffer(&data_ptr);
		*pic = data_ptr;
	}
	else {
		*pic = NULL;
	}
}

void CVLCControl::unlock_callback_internal(void *pic, void * const *plane)
{
	if (NULL != _callback_ptr) {
		_callback_ptr->on_unlock_buffer();
	}
}

void CVLCControl::display_callback_internal(void *pic)
{
	// NOTICE:
	// Version <= 2.1.5: lock -> display -> unlock.(normal resolution and infos.)
	// Version > 2.1.5: lock -> unlock -> display.(abnormal resolution and infos.)

	if (NULL != _callback_ptr) {
		_callback_ptr->on_display();
	}
}

unsigned CVLCControl::setup_callback_internal(char *chroma, unsigned *width,
	unsigned *height, unsigned *pitches, unsigned *lines)
{
	/*unsigned int num = 0, width_rel = *width, height_rel = *height;
	if (0 == libvlc_video_get_size(_player_ptr, num, &width_rel, &height_rel)) {
		*width = width_rel;
		*height = height_rel;
	}
	*/
	uint32_t width_rel = *width;
	uint32_t height_rel = *height;
	chroma[0] = 'R';
	chroma[1] = 'V';
	chroma[2] = '2';
	chroma[3] = '4';
	*pitches = width_rel * 3;
	*lines = height_rel;
	if (NULL != _callback_ptr) {
		_callback_ptr->on_setup(width_rel, height_rel);
	}

	return 1;
}

void CVLCControl::clearup_callback_internal()
{
	if (NULL != _callback_ptr) {
		_callback_ptr->on_clearup();
	}
}

int CVLCControl::mem_open_callback_internal(uint64_t *size_ptr)
{
	if (NULL != _mem_callback_ptr) {
		return _mem_callback_ptr->on_mem_open(size_ptr);
	}

	return -1;
}

size_t CVLCControl::mem_read_callback_internal(unsigned char *buf, size_t len)
{
	if (NULL != _mem_callback_ptr) {
		return _mem_callback_ptr->on_mem_read(buf, len);
	}

	return 0;
}

int CVLCControl::mem_seek_callback_internal(uint64_t offset)
{
	if (NULL != _mem_callback_ptr) {
		return _mem_callback_ptr->on_mem_seek(offset);
	}

	return -1;
}

void CVLCControl::mem_close_callback_internal()
{
	if (NULL != _mem_callback_ptr) {
		_mem_callback_ptr->on_mem_close();
	}
}

