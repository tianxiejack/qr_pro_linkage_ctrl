#include "globalDate.h"

CGlobalDate* CGlobalDate::_Instance = 0;

vector<int>  CGlobalDate::defConfigBuffer;

vector<Read_config_buffer>  CGlobalDate::readConfigBuffer;

osdbuffer_t CGlobalDate::recvOsdbuf = {0};

CGlobalDate::CGlobalDate():EXT_Ctrl(Cmd_Mesg_Max), Host_Ctrl(40), commode(2), feedback(0), choose(0), IrisAndFocus_Ret(0), time_stop(0), frame(0)
{
	jos_ctrl = host_ctrl = 0;
	joystick_flag = 0;
	 workMode = 0;
	outputMode = 0;
	target = time_start = respupgradefw_stat = respupgradefw_perc = respexpconfig_stat = respexpconfig_len = ImgMtdStat = mtdMode = ThreeMode_bak = 0;
	memset(respexpconfig_buf, 0, sizeof(respexpconfig_buf));
	Mtd_Limit = recv_AutoMtdDate;
	Sync_Query = 0;
	Mtd_Moitor = Mtd_Moitor_X = Mtd_Moitor_Y = 0;
	Mtd_ipc_target = 0;
	Mtd_Manual_Limit = 0;
	Mtd_ExitMode = 10;
	rcv_zoomValue = 0;
	MtdAutoLoop = false;
	memset(&ipc_mouseptz, 0, sizeof(ipc_mouseptz));
	memset(&mtdconfig, 0, sizeof(mtdconfig));
	memset(&mainProStat, 0, sizeof(mainProStat));
	mtdconfig.preset = 1;
}

CGlobalDate* CGlobalDate::Instance()
{
	if(_Instance == 0)
		_Instance =  new CGlobalDate();
	return _Instance;
}
