首先介绍两个文件

```
/etc/fstab
/etc/mtab
```

fstab是系统分区信息以及系统启动时磁盘的挂载参数，该文件是一个静态文件（系统启动后不再改变，如人为改变，需要重启系统）
mtab是当前系统中已经挂载的磁盘列表，该文件是一个动态文件，即随系统mount和umount文件系统而随时发生改变

```
cat fstab
# /etc/fstab: static file system information.
#
# Use 'blkid' to print the universally unique identifier for a
# device; this may be used with UUID= as a more robust way to name devices
# that works even if disks are added and removed. See fstab(5).
#
# <file system> <mount point>   <type>  <options>       <dump>  <pass>
# / was on /dev/sda1 during installation
UUID=85b40d1f-fd6c-44cc-9f29-4b3c4f5c6268 /               ext4    errors=remount-ro 0       1
# swap was on /dev/sda5 during installation
UUID=eb5303a8-ecad-4b3a-a7e7-5f1c74ec4222 none            swap    sw              0       0
/dev/sdc1 /media/sdb1 fuseblk uid=1000,gid=1000,umask=000,blksize=4096 0 0
```

```
[15:50@/etc]> cat mtab
/dev/sda1 / ext4 rw,errors=remount-ro 0 0
proc /proc proc rw,noexec,nosuid,nodev 0 0
sysfs /sys sysfs rw,noexec,nosuid,nodev 0 0
none /sys/fs/cgroup tmpfs rw 0 0
none /sys/fs/fuse/connections fusectl rw 0 0
none /sys/kernel/debug debugfs rw 0 0
none /sys/kernel/security securityfs rw 0 0
udev /dev devtmpfs rw,mode=0755 0 0
devpts /dev/pts devpts rw,noexec,nosuid,gid=5,mode=0620 0 0
tmpfs /run tmpfs rw,noexec,nosuid,size=10%,mode=0755 0 0
none /run/lock tmpfs rw,noexec,nosuid,nodev,size=5242880 0 0
none /run/shm tmpfs rw,nosuid,nodev 0 0
none /run/user tmpfs rw,noexec,nosuid,nodev,size=104857600,mode=0755 0 0
none /sys/fs/pstore pstore rw 0 0
binfmt_misc /proc/sys/fs/binfmt_misc binfmt_misc rw,noexec,nosuid,nodev 0 0
systemd /sys/fs/cgroup/systemd cgroup rw,noexec,nosuid,nodev,none,name=systemd 0 0
gvfsd-fuse /run/user/1000/gvfs fuse.gvfsd-fuse rw,nosuid,nodev,user=wangxu 0 0
/dev/sdb1 /media/wangxu fuseblk rw,nosuid,nodev,allow_other,blksize=4096 0 0
```
主要看mtab文件，这个文件中的最后一行表示的是我们使用的移动硬盘
第一个参数是file system ，第二个参数是mount point

我们的做法就是先使用 unmount ，在mount，并且加上x可执行权限

sudo umount /media/wangxu/
sudo mount /dev/sdb1 /media/wangxu/ -o rwx

主要看-o指定options ， 这边添加的 x，即执行权限

