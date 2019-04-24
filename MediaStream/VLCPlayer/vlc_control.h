#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include "vlc/vlc.h"


/**
* @brief:
* VLC control class.
* Time is all in msec.
* The time changed callback is not correct when playing from memory.(Works well when playing from file.)
*/
class CVLCCallback
{
public:
	virtual void on_setup(uint32_t width, uint32_t height) = 0; // For buffer new
	virtual void on_clearup() = 0;								// For buffer delete
	virtual void on_lock_buffer(uint8_t **data_ptr) = 0;		// For buffer lock
	virtual void on_unlock_buffer() = 0;						// For buffer unlock
	virtual void on_display() = 0;								// For buffer display
	virtual void on_play_time_changed(uint32_t time_ms) = 0;	// Notify for time changed
	virtual void on_play_finished() = 0;						// Notify for ending
};


class CVLCMemCallback
{
public:
	virtual int on_mem_open(uint64_t *size_ptr) = 0;
	virtual size_t on_mem_read(uint8_t *buf, size_t len) = 0;	// Should sleep when no available.
	virtual int on_mem_seek(uint64_t offset) = 0;
	virtual void on_mem_close() = 0;
};


class CVLCControl
{
public:
	CVLCControl();
	virtual ~CVLCControl();

	bool create(CVLCCallback *callback_ptr);
	void destroy();

	bool open_file(const char *path_ptr);
	void close_file();

	bool open_mem(CVLCMemCallback *mem_callback);
	void close_mem();

	bool start();
	void pause();
	void stop();
	int get_state();
	int get_length();
	int get_current();
	void set_current(int time_ms);
	int get_volume();
	void set_volume(int vol);

	static void events_callback(const libvlc_event_t *evt_ptr, void *param)
	{
		CVLCControl *this_ptr = (CVLCControl *)param;
		if (NULL != this_ptr)
			this_ptr->events_callback_internal(evt_ptr);
	}
	void events_callback_internal(const libvlc_event_t *evt_ptr);
	
	static void* lock_callback(void *param, void **pic)
	{
		CVLCControl *this_ptr = (CVLCControl *)param;
		if (NULL != this_ptr)
			this_ptr->lock_callback_internal(pic);

		return NULL;
	}
	void lock_callback_internal(void **pic);

	static void unlock_callback(void *param, void *pic, void * const *plane)
	{
		CVLCControl *this_ptr = (CVLCControl *)param;
		if (NULL != this_ptr)
			this_ptr->unlock_callback_internal(pic, plane);
	}
	void unlock_callback_internal(void *pic, void * const *plane);

	static void display_callback(void *param, void *pic)
	{
		CVLCControl *this_ptr = (CVLCControl *)param;
		if (NULL != this_ptr)
			this_ptr->display_callback_internal(pic);
	}
	void display_callback_internal(void *pic);

	static unsigned setup_callback(void **param, char *chroma, unsigned *width,
		unsigned *height, unsigned *pitches, unsigned *lines)
	{
		CVLCControl *this_ptr = (CVLCControl *)(*param);
		if (NULL != this_ptr)
			this_ptr->setup_callback_internal(chroma, width, height, pitches, lines);

		return 1;
	}
	unsigned setup_callback_internal(char *chroma, unsigned *width,
		unsigned *height, unsigned *pitches, unsigned *lines);

	static void clearup_callback(void *param)
	{
		CVLCControl *this_ptr = (CVLCControl *)param;
		if (NULL != this_ptr)
			this_ptr->clearup_callback_internal();
	}
	void clearup_callback_internal();

	// For memory
	static int mem_open_callback(void *param, void **user_ptr, uint64_t *size_ptr)
	{
		*user_ptr = param;
		CVLCControl *this_ptr = (CVLCControl *)param;
		if (NULL != this_ptr)
			return this_ptr->mem_open_callback_internal(size_ptr);

		return -1;
	}
	int mem_open_callback_internal(uint64_t *size_ptr);

	static size_t mem_read_callback(void *user_ptr, unsigned char *buf, size_t len) // param is passed by libvlc_media_open_cb
	{
		CVLCControl *this_ptr = (CVLCControl *)user_ptr;
		if (NULL != this_ptr)
			return this_ptr->mem_read_callback_internal(buf, len);

		return 0;
	}
	size_t mem_read_callback_internal(unsigned char *buf, size_t len);

	static int mem_seek_callback(void *user_ptr, uint64_t offset)
	{
		CVLCControl *this_ptr = (CVLCControl *)user_ptr;
		if (NULL != this_ptr)
			return this_ptr->mem_seek_callback_internal(offset);

		return -1;
	}
	int mem_seek_callback_internal(uint64_t offset);

	static void mem_close_callback(void *user_ptr)
	{
		CVLCControl *this_ptr = (CVLCControl *)user_ptr;
		if (NULL != this_ptr)
			this_ptr->mem_close_callback_internal();
	}
	void mem_close_callback_internal();


protected:
	CVLCMemCallback *_mem_callback_ptr;
	CVLCCallback *_callback_ptr;

	libvlc_instance_t *_instance_ptr;
	libvlc_media_player_t *_player_ptr;
	libvlc_media_t *_media_ptr;
	libvlc_event_manager_t *_event_ptr;
};

