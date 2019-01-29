#include "PlatformInterface.h"
#include "platformControl.h"
#include "manager.h"


int main (int argc, char** argv)
{
	struct timeval tmp;
	CManager* _Manager = new CManager();

	while(1)
	{
		tmp.tv_sec = 0;
		tmp.tv_usec = 10000;
		select(0, NULL, NULL, NULL, &tmp);
	}

	return 0;
}
