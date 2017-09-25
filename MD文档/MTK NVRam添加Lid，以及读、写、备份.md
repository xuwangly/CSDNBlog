具体流程分为三步
1、添加一个新的Lid，用来保存数据
2、在userspace进行读写备份操作
3、往上提供接口

1.添加Lid，MTK有相关文档直接按照说明操作即可，不同的版本可能有些区别
这边我使用的MT8163，android6.0的版本，贴出一个diff文件供大家参考
[Diff文件链接NvRam.diff](https://github.com/xuwangly/FIXED/blob/master/NVRAM/NvRam.diff)

2.操作NVram，主要需要两个so库。libnvram libfile_op，代码如下可供参考
Android.mk

```
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := backnvram.cpp
LOCAL_C_INCLUDES := $(MTK_PATH_SOURCE)/external/nvram/libnvram $(MTK_PATH_SOURCE)/kernel/include/ $(MTK_PATH_SOURCE)/external/nvram/libfile_op $(MTK_PATH_CUSTOM)/cgen/inc $(MTK_PATH_CUSTOM)/cgen/cfgfileinc $(MTK_PATH_CUSTOM)/cgen/cfgdefault
LOCAL_SHARED_LIBRARIES := libnvram libfile_op libutils liblog
#LOCAL_STATIC_LIBRARIES := 
LOCAL_MODULE := nvramtest
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_MODULE_TAGS := optional
#LOCAL_PRELINK_MODULE := false
#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)
include $(BUILD_EXECUTABLE)

```
NVRamTest.cpp

```
#include "libnvram.h"
#include "libfile_op.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "Custom_NvRam_LID.h"
 
char buf[491520] = {0};

int test_read()
{
    int rec_size=0 , rec_num = 0 , result;
    float buf[20] = {0};
    F_ID f_id = NVM_GetFileDesc(AP_CFG_CUSTOM_FILE_CUSTOM1_LID, &rec_size, &rec_num,1);
    result = read(f_id.iFileDesc,&buf,rec_num*rec_size);
    printf("read-result:%d\n" , result);
    printf("read:%s\n" ,buf);
	for(int i ; i < 9 ; i++)
		printf("%f\n" , buf[i]);
    NVM_CloseFileDesc(f_id);
    printf("-----------------%d-------------------size:%d*%d=%d\n",AP_CFG_CUSTOM_FILE_CUSTOM1_LID, rec_size ,rec_num ,rec_num*rec_size);
    return 0;
} 
int test_write()
{
    int rec_size=0 , rec_num = 0 ,  result;
    char buf[400];
    F_ID f_id = NVM_GetFileDesc(AP_CFG_CUSTOM_FILE_CUSTOM1_LID, &rec_size, &rec_num,0);
    memset(buf , 'Z' , sizeof(buf));
    buf[AP_CFG_CUSTOM_FILE_CUSTOM1_LID-1] = '\n';
    result = write(f_id.iFileDesc,&buf,rec_num*rec_size);
    printf("write:%d\n" ,result);
    NVM_CloseFileDesc(f_id);
    int ret = FileOp_BackupToBinRegion_All();
    printf("FileOp_BackupToBinRegion_All return%d\n", ret);
    
    printf("-----------------%d-------------------size:%d*%d=%d\n",AP_CFG_CUSTOM_FILE_CUSTOM1_LID, rec_size ,rec_num ,rec_num*rec_size);
    return 0;
}
int main(int argc, const char** argv)
{
	printf("AP_CFG_CUSTOM_FILE_CUSTOM1_LID=%d\n" , AP_CFG_CUSTOM_FILE_CUSTOM1_LID);	
	test_write();
	test_read();
}
```

3.第三步，一般我们添加Nvram的接口主要给java层调用，一般通过JNI接口，但是使用jNI调用会出现失败。这是因为android的SELinux权限机制，可以添加一个daemon放到init.rc中（放到init.rc也需要给SELinux权限），通过socket和java通信。这部分以后有时间再试咯。