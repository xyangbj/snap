#!/bin/bash
#
# Copyright 2017, International Business Machines
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# Simple tests for example snap actions.
#

verbose=0
snap_card=0
iteration=1
FUNC="./actions/hdl_example/sw/snap_example_ddr"

function test_ddr ()	# $1 = card, $2 = start, $2 = end, $3 = block size
{
	local card=$1
	local start=$2
	local end=$3
	local block_size=$4

	echo "Using Polling Mode"
	cmd="$FUNC -v -C ${card} -s $start -e $end -b $block_size"
	eval ${cmd}
	if [ $? -ne 0 ]; then
		echo -n "Error: cmd: <${cmd}>"
		echo " failed"
		exit 1
	fi
	echo "Using IRQ Mode"
	cmd="$FUNC -v -C ${card} -s $start -e $end -b $block_size -I"
	eval ${cmd}
	if [ $? -ne 0 ] ; then
		echo -n "Error: cmd: <${cmd}>"
		echo " failed"
		exit 1
	fi
}

function usage() {
	echo "SNAP Example Action 10140000 SDRAM Test"
	echo "Usage:"
	echo "  $PROGRAM"
	echo "    [-C <card>]        card to be used for the test"
	echo "    [-t <trace_level>]"
	echo "    [-i <iteration>]"
}

#
# main starts here
#
PROGRAM=$0

while getopts "C:t:i:h" opt; do
	case $opt in
	C)
	snap_card=$OPTARG;
	;;
	t)
	SNAP_TRACE=$OPTARG;
	;;
	i)
	iteration=$OPTARG;
	;;
	h)
	usage;
	exit 0;
	;;
	\?)
	echo "Invalid option: -$OPTARG" >&2
	;;
	esac
done

rev=$(cat /sys/class/cxl/card$snap_card/device/subsystem_device | xargs printf "0x%.4X")

case $rev in
"0x0605" )
        echo "$rev -> Testing AlphaDataKU60 Card"
        ;;
"0x060A" )
        echo "$rev -> Testing FlashGT Card"
        ;;
*)
        echo "Capi Card $snap_card does have subsystem_device: $rev"
        echo "I Expect to have 0x605 or 0x60a, Check if -C $snap_card was"
        echo " move to other CAPI id and use other -C option !"
        exit 1
esac;

# Get RAM in MB from Card
RAM=`./software/tools/snap_maint -C $snap_card -m 3`
if [ -z $RAM ]; then
	echo "Skip Test: No SRAM on Card $snap_card"
	exit 0
fi

KB=$((1024))
MB=$((1024*1024))
GB=$((1024*1024*1024))
BLOCKSIZE=$((1*MB))
echo "Testing $RAM MB SRAM on Card $snap_card"

for ((iter=1;iter <= iteration;iter++))
{
	if [ "$RAM" -ge "1024" ]; then
		echo "Memory Test $iter of $iteration (1GB)"
		start=0
		size=$((1*$GB))
		end=$(($start+$size))
		test_ddr "${snap_card}" "${start}" "${end}" "${BLOCKSIZE}"
	fi
	if [ "$RAM" -ge "2048" ]; then
		echo "Memory Test $iter of $iteration (1GB)"
		start=${end}
		end=$(($start+1*$GB))
		test_ddr "${snap_card}" "${start}" "${end}" "${BLOCKSIZE}"
	fi
	if [ "$RAM" -ge "3072" ]; then
		echo "Memory Test $iter of $iteration (1GB)"
		start=${end}
		end=$(($start+$GB))
		test_ddr "${snap_card}" "${start}" "${end}" "${BLOCKSIZE}"
	fi
	if [ "$RAM" -ge "4096" ]; then
		echo "Memory Test $iter of $iteration (1GB)"
		start=${end}
		end=$(($start+1*$GB))
		test_ddr "${snap_card}" "${start}" "${end}" "${BLOCKSIZE}"
	fi
	if [ "$RAM" -ge "4096" ]; then
		echo "Memory Test $iter of $iteration (2 x 2GB)"
		start=0
		size=$((2*$GB))
		end=$(($start+$size))
		test_ddr "${snap_card}" "${start}" "${end}" "${BLOCKSIZE}"
		start=${end}
		end=$(($start+$size))
		test_ddr "${snap_card}" "${start}" "${end}" "${BLOCKSIZE}"
	fi
	if [ "$RAM" -ge "4096" ]; then
		echo "Memory Test $iter of $iteration (1 x 4GB)"
		start=0
		size=$((4*$GB))
		end=$(($start+$size))
		test_ddr "${snap_card}" "${start}" "${end}" "${BLOCKSIZE}"
	fi
	if [ "$RAM" -ge "8192" ]; then
		echo "Memory Test $iter of $iteration (1 x 8GB)"
		size=$((8*$GB))
		start=0
		end=$(($start+$size))
		test_ddr "${snap_card}" "${start}" "${end}" "${BLOCKSIZE}"
	fi
}
exit 0
