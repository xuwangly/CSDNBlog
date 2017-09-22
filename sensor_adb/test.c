#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/mtgpio.h>
#include <cutils/log.h>

#include "libhwm.h"

static int fd = -1 ;
static int period = 50 ;
static int count = 20 ;
static int tolerance = 20 ;
//static int tolerance = 40 ;
static int trace = 0 ;
static HwmData cali ;
static HwmData cali_drv ;
static HwmData cali_nvram ;
static HwmData cali_gyro;

int cali_gsensor();
int cali_gyroscope();

int main(int argc, const char** argv)
{
    int ret = 0;
	int i = 0 ;
	for(i = 0  ; i < 2 ; i++){
		if(i){
			ret += cali_gyroscope();
		}
		else{
			ret += cali_gsensor();
		}
	}
	if(ret){
	    printf("\n\n-----sensor校准失败！请重新校准！-----\n\n");
		return -1;
	}
	else
	    printf("\n\n-----sensor校准成功！-----\n\n");
	return 0 ;
}
int cali_gsensor()
{
	fd = -1;
	int i = 0;
	gsensor_open(&fd);
	if(fd < 0)
	{
		printf("Open gsensor device error!\n");
		goto error;
	}
	printf("Open gsensor device succeed! fd:%d \n", fd);

	i = gsensor_calibration(fd, period, count, tolerance, trace, &cali);
	if(i != 0){
		printf("gsensor_calibration error\n");
		goto error;
	}
	printf("\n");

	i += gsensor_set_cali(fd, &cali);
	if(i != 0){
		printf("gsensor_set_cali error\n");
		goto error;
	}
	printf("\n");

	i += gsensor_get_cali(fd, &cali);
	if(i != 0){
		printf("gsensor_get_cali error\n");
		goto error;
	}
	printf("\n");

	i += gsensor_write_nvram(&cali);
	if(i != 0){
		printf("gsensor_write_nvram error\n");
		goto error;
	}
	printf("gsensor calibration success! \n",i );
	gsensor_close(fd);
	return 0;
error:
	return -1;	
}
int cali_gyroscope()
{
	fd = -1;
	int i = 0;
	gyroscope_open(&fd);
	if(fd < 0)
	{
		printf("gyroscope_open device error!\n");
		goto error;
	}
	printf("gyroscope_open succeed! fd:%d \n", fd);

	i = gyroscope_calibration(fd, period, count, tolerance*10, trace, &cali_gyro);
	if(i != 0){
		printf("gyroscope_calibration error\n");
		goto error;
	}
	printf("\n");

	i += gyroscope_set_cali(fd, &cali_gyro);
	if(i != 0){
		printf("gyroscope_set_cali error\n");
		goto error;
	}
	printf("\n");

	i += gyroscope_get_cali(fd, &cali_gyro);
	if(i != 0){
		printf("gyroscope_get_cali error\n");
		goto error;
	}
	printf("\n");

	i += gyroscope_write_nvram(&cali_gyro);
	if(i != 0){
		printf("gyroscope_write_nvram error\n");
		goto error;
	}
	printf("gyroscope calibration success! \n",i );
	return 0;
error:
	return -1;	
}
