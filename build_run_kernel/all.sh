#!/bin/bash

set -e

cur_dir=$(cd $(dirname $0); pwd)
src_dir=${cur_dir}/src

function usage()
{
    echo "$0    [-k kernel-version] [-b busybox-version] [-h]"
    echo
    echo "options:"
    echo "    -k kernel-version       Specify kernel version, default is 4.15.1"
    echo "    -b busybox-version      Seecify busybox version, default is 1.34.1"
    echo "    -h                      Show this help message"
}

kernel_ver="4.15.1"
busybox_ver="1.34.1"
while getopts ":k:b:h" opt; do
    case $opt in
        k)  
            kernel_ver=$OPTARG
            ;;
        b)  
            busybox_ver=$OPTARG
            ;;
        h)  
            usage
            exit 0
            ;;
        \?) 
            echo "Invalid option: -$OPTARG" >&2
            usage
            exit 1
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            usage
            exit 1
            ;;
    esac
done

kernel_file="linux-${kernel_ver}.tar.xz"
kernel_path="${src_dir}/${kernel_file}"

build_dir=$cur_dir/build
kernel_dir=$build_dir/${kernel_file%%.tar.xz}
kernel_image=$kernel_dir/arch/x86/boot/bzImage

function download_kernel()
{
    [ ! -d $src_dir ] && mkdir -p $src_dir
    base_url="https://mirrors.aliyun.com/linux-kernel"
    major_ver=${kernel_ver%%\.*}
    kernel_url="${base_url}/v${major_ver}.x/${kernel_file}"
    wget -P $src_dir $kernel_url
}

function build_kernel()
{
    [ ! -e $kernel_path ] && download_kernel
    [ ! -d $build_dir ] && mkdir -p $build_dir
    tar xvf $kernel_path -C $build_dir
    cd $kernel_dir
    make defconfig
    make -j$(nproc)
}

busybox_file="busybox-${busybox_ver}.tar.bz2"
busybox_path="${src_dir}/${busybox_file}"
busybox_dir="${build_dir}/${busybox_file%%.tar.bz2}"
function download_busybox()
{
    [ ! -d $src_dir ] && mkdir -p $src_dir
    busybox_url="https://busybox.net/downloads/${busybox_file}"
    wget -P $src_dir $busybox_url
}

function build_busybox()
{
    [ ! -e $busybox_path ] && download_busybox
    [ ! -d $build_dir ] && mkdir -p $build_dir 
    tar xvf $busybox_path -C $build_dir
    # 在menuconfig配置界面，设置Settings -> Build options -> uild static binary (no shared libs)
    cd $busybox_dir && make menuconfig && make -j$(nproc)
}

disk_dir=$cur_dir/disks
disk_file="disk-${kernel_ver}"
disk_path=$disk_dir/$disk_file
rootfs_dir=$disk_dir/rootfs
function build_disk()
{
    [ ! -d $disk_dir ] && mkdir -p $disk_dir
    [ ! -d $rootfs_dir ] && mkdir -p $rootfs_dir
    cd $disk_dir
    qemu-img create -f raw $disk_file 512M
    mkfs -t ext4 $disk_file
    mount -o loop $disk_file $rootfs_dir

    # 安装模块
    cd $kernel_dir && make modules_install INSTALL_MOD_PATH=$rootfs_dir

    # 安装根文件系统
    cd $busybox_dir && make install CONFIG_PREFIX=$rootfs_dir

    # 制作initd
    cd $rootfs_dir
    mkdir -p {etc,etc/init.d,dev,mnt,proc,sys,tmp}
    
    touch etc/fstab
    cat <<EOF >etc/fstab
proc    /proc   proc    defaults    0   0
tmpfs   /tmp    tmpfs   defaults    0   0
sysfs   /sys    sysfs   defaults    0   0
EOF

    touch etc/init.d/rcS
    cat <<EOF >etc/init.d/rcS
echo -e "Welcom to tinyLinux"
/bin/mount -a
echo -e "Remounting the root filesystem"
mount -o remount,rw /
mkdir -p /dev/pts
mount -t devpts devpts /dev/pts
echo /sbin/mdev >/proc/sys/kernel/hotplug
mdev -s
EOF
    chmod 755 etc/init.d/rcS

    touch etc/inittab
    cat <<EOF >etc/inittab
::sysinit:/etc/init.d/rcS
::respawn:-/bin/sh
::askfirst:-/bin/sh
::ctrlaltdel:/bin/umount -a -r
EOF
    chmod 755 etc/inittab

    cd dev
    mknod console c 5 1
    mknod null c 1 3
    mknod tty1 c 4 1

    sync
    sleep 5
}

# 编译内核
[ ! -e $kernel_image ] && build_kernel

# 编译根文件系统
[ ! -d $busybox_dir ] && build_busybox

# 制作磁盘及其在磁盘中制作根文件系统
[ ! -e $disk_path ] && build_disk

# 现在，可以通过qemu启动内核系统了
qemu-system-x86_64 -m 512M -kernel $kernel_image -append "root=/dev/sda init=linuxrc" -drive format=raw,file=$disk_path -serial file:output.txt