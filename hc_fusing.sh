#!/bin/bash
#
# Copyright (C) 2011 Samsung Electronics Co., Ltd.
#              http://www.samsung.com/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#

if [ -z $1 ]; then
	echo "usage: ./hc_fusing.sh <SD Reader's device file>"
	exit 0
fi

if [ -b $1 ]; then
	echo "$1 reader is identified."
else
	echo "$1 is NOT identified."
	exit 0
fi

####################################
#<verify device>

BDEV_NAME=`basename $1`
BDEV_SIZE=`cat /sys/block/${BDEV_NAME}/size`

if [ ${BDEV_SIZE} -le 0 ]; then
	echo "Error: NO media found in card reader."
	exit 1
fi

if [ ${BDEV_SIZE} -gt 32000000 ]; then
	echo "Error: Block device size (${BDEV_SIZE}) is too large"
	exit 1
fi

####################################
# check files

ZIMAGE=arch/arm/boot/zImage

if [ ! -f ${ZIMAGE} ]; then
	echo "Error: zImage NOT found, please build it & try again."
	exit -1
fi

####################################
# fusing images

let BL1_POSITION=${BDEV_SIZE}-16-2-1024
let BL2_POSITION=${BDEV_SIZE}-16-2-1024-32-512
let KERNEL_POSITION=${BL2_POSITION}-12288
#echo ${KERNEL_POSITION}

umount /dev/${DEV_NAME}* > /dev/null 2>&1

#<zImage fusing>
echo "---------------------------------------"
echo "Kernel fusing"
dd if=${ZIMAGE} of=$1 bs=512 seek=${KERNEL_POSITION}

#<flush to disk>
sync

####################################
#<Message Display>
echo "---------------------------------------"
echo "Kernel zImage is fused (at `date +%T`) successfully."
echo "Eject SD card and insert it again."

