#pragma once
#include <stdint.h>


///////////////////////////////////////////////////////
// Video
typedef enum _video_fourcc
{
	VIDEO_DOURCC_UNKNOWN = 0x00,
	VIDEO_FOURCC_RGBA,
	VIDEO_FOURCC_BGRA,
	VIDEO_FOURCC_ARGB,
	VIDEO_FOURCC_ABGR,
	VIDEO_FOURCC_RGB,
	VIDEO_FOURCC_BGR,
	VIDEO_FOURCC_YUV420,
	VIDEO_FOURCC_NV12
} video_fourcc_t;

typedef enum _video_encoder
{
	VIDEO_ENCODER_DEFAULT = 0x00,
	VIDEO_ENCODER_X264,
	VIDEO_ENCODER_NVIDIA,
	VIDEO_ENCODER_INTEL
} video_encoder_t;

typedef enum _video_codec
{
	VIDEO_CODEC_UNKNOWN = 0x00,
	VIDEO_CODEC_H264,
	VIDEO_CODEC_HEVC
} video_codec_t;

typedef enum _video_preset
{
	VIDEO_PRESET_DEFAULT = 0x00,
	VIDEO_PRESET_SPEED,
	VIDEO_PRESET_BALANCED,
	VIDEO_PRESET_HIGHQUALITY
} video_preset_t;

typedef enum _video_profile
{
	VIDEO_PROFILE_DEFAULT = 0x00,
	VIDEO_PROFILE_BASELINE,
	VIDEO_PROFILE_MAIN,
	VIDEO_PROFILE_HIGH
} video_profile_t;

typedef struct _video_enc_config
{
	uint32_t device_id;
	void *device_ptr;

	video_codec_t codec;
	video_preset_t preset;
	video_profile_t profile;
	video_fourcc_t fourcc;
	uint32_t width;
	uint32_t height;
	uint32_t fps_num;
	uint32_t fps_den;
	uint32_t bitrate;
	uint32_t gop;
	uint32_t b_frames;
} video_enc_config_t;

#define VIDEO_ENC_PARAM_LEN			512
#define VIDEO_ENC_PARAM_LEN_LONG	1024
typedef struct _video_enc_param
{
	uint32_t size_vps;
	uint8_t data_vps[VIDEO_ENC_PARAM_LEN];
	uint32_t size_sps;
	uint8_t data_sps[VIDEO_ENC_PARAM_LEN];
	uint32_t size_pps;
	uint8_t data_pps[VIDEO_ENC_PARAM_LEN];
	uint32_t size_sei_prefix;
	uint8_t data_sei_prefix[VIDEO_ENC_PARAM_LEN_LONG];
	uint32_t size_sei_suffix;
	uint8_t data_sei_suffix[VIDEO_ENC_PARAM_LEN_LONG];
} video_enc_param_t;


///////////////////////////////////////////////////////
// Audio
typedef enum _audio_fourcc
{
	AUDIO_FOURCC_UNKNOWN = 0x00,
	AUDIO_FOURCC_U8,
	AUDIO_FOURCC_S16,
	AUDIO_FOURCC_S32,
	AUDIO_FOURCC_U8P,
	AUDIO_FOURCC_S16P,
	AUDIO_FOURCC_S32P,
	AUDIO_FOURCC_FLT,
	AUDIO_FOURCC_FLTP
} audio_fourcc_t;

typedef struct _audio_enc_config
{
	uint32_t bitrate;
	uint8_t bitdepth;
	audio_fourcc_t fourcc;
	uint32_t channels;
	uint32_t samplerate;
	uint32_t samples_per_frame;
} audio_enc_config_t;

#define AUDIO_ENC_PARAM_LEN		64
typedef struct _audio_enc_param
{
	uint32_t size_esds;
	uint8_t data_esds[AUDIO_ENC_PARAM_LEN];
} audio_enc_param_t;


///////////////////////////////////////////////////////
// Frame/Publish
typedef struct _frame_info
{
	bool is_video;
	bool key_frame;
	uint64_t pts;
	uint64_t dts;
} frame_info_t;

typedef struct _publish_info
{
	// Video
	uint32_t width;
	uint32_t height;
	float fps;
	float bitrate;
	video_enc_param_t params;

	// Audio
	bool has_audio;
	uint32_t channels;
	uint32_t samplerate;
	uint32_t samples_per_frame;
	uint32_t datarate;
} publish_info_t;


///////////////////////////////////////////////////////
// Extras
typedef enum _nal_unit_type
{
	NAL_TYPE_TRAIL_N = 0,
	NAL_TYPE_TRAIL_R = 1,
	NAL_TYPE_TSA_N = 2,
	NAL_TYPE_TSA_R = 3,
	NAL_TYPE_STSA_N = 4,
	NAL_TYPE_STSA_R = 5,
	NAL_TYPE_RADL_N = 6,
	NAL_TYPE_RADL_R = 7,
	NAL_TYPE_RASL_N = 8,
	NAL_TYPE_RASL_R = 9,
	NAL_TYPE_BLA_W_LP = 16,
	NAL_TYPE_BLA_W_RADL = 17,
	NAL_TYPE_BLA_N_LP = 18,
	NAL_TYPE_IDR_W_RADL = 19,
	NAL_TYPE_IDR_N_LP = 20,
	NAL_TYPE_CRA_NUT = 21,
	NAL_TYPE_VPS = 32,
	NAL_TYPE_SPS = 33,
	NAL_TYPE_PPS = 34,
	NAL_TYPE_AUD = 35,
	NAL_TYPE_EOS_NUT = 36,
	NAL_TYPE_EOB_NUT = 37,
	NAL_TYPE_FD_NUT = 38,
	NAL_TYPE_SEI_PREFIX = 39,
	NAL_TYPE_SEI_SUFFIX = 40
} nal_unit_type_t;






