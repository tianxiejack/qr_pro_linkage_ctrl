#ifndef _APP_STATUS_H
#define _APP_STATUS_H

typedef enum {
	eSen_CH0	= 0x00,
	eSen_CH1	= 0x01,
	eSen_CH2   = 0x02,
	eSen_CH3 	= 0x03,
	eSen_CH4   = 0x04,
	eSen_CH5	= 0x05,
	eSen_Max   = 0x06
}eSenserStat;

typedef enum {
	Trk_Size0	= 0x00,
	Trk_Size1	= 0x01,
	Trk_Size2   = 0x02,
	Trk_Size3 	= 0x03,
	Trk_Size4	= 0x04,
	Trk_SizeMax 	= 0x05
}eTrkAimSize;

enum devvideo{
	video_gaoqing0=0,
	video_gaoqing,
	video_gaoqing2,
	video_gaoqing3,
	video_pal,
	MAX_CHAN,
};

static int ShowDPI[MAX_CHAN][2] = {{1920, 1080},{1920, 1080},{1920, 1080},{1920, 1080},{720, 576}};

#endif
