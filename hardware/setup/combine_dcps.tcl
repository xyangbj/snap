############################################################################
############################################################################
##
## Copyright 2016,2017 International Business Machines
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions AND
## limitations under the License.
##
############################################################################
############################################################################

set root_dir    $::env(SNAP_HARDWARE_ROOT)
set fpga_part   $::env(FPGACHIP)
set fpga_card   $::env(FPGACARD)
set log_dir     $::env(LOGS_DIR)
set log_file    $log_dir/combine_dcps.log

if { [info exists ::env(DCP_ROOT)] == 1 } {
    set dcp_dir $::env(DCP_ROOT)
    set file_missing 0
    if { [file exists $dcp_dir/snap_static_region_bb.dcp] != 1 } {
        puts "Error: File \$DCP_ROOT/snap_static_region_bb.dcp does not exist"
        set file_missing 1
    }
    if { [file exists $dcp_dir/user_action.dcp] != 1 } {
        puts "Error: File \$DCP_ROOT/user_action.dcp does not exist"
        set file_missing 1
    }
    if { $file_missing == 1 } {
        exit 42
    }
} else {
    puts "Error: For combining DCPs the environment variable DCP_ROOT needs to point to the path containing the checkpoints."
    exit 42
}


# Create a new Vivado Project
puts "	\[COMBINE_DCPS........\] start `date`"
create_project combine_dcps $root_dir/viv_project -part $fpga_part -force >> $log_file

# Project Settings
# General
puts "	                        setting up project settings"
set_property target_language VHDL [current_project]
set_property default_lib work [current_project]

# Bitstream
#set_property STEPS.WRITE_BITSTREAM.TCL.PRE  $root_dir/setup/snap_bitstream_pre.tcl  [get_runs impl_1]
#set_property STEPS.WRITE_BITSTREAM.TCL.POST $root_dir/setup/snap_bitstream_post.tcl [get_runs impl_1]

# Opening PSL/SNAP static region checkpoint
puts "	                        opening PSL/SNAP static region checkpoint"
open_checkpoint $dcp_dir/snap_static_region_bb.dcp >> $log_file

# Adding Action checkpoint
puts "	                        adding Action design checkpoint"
read_checkpoint -cell a0/action_w $dcp_dir/user_action.dcp -strict >> $log_file

# Generating bitstream
puts "	                        writing bitstream"
#source $root_dir/setup/snap_bitstream_pre.tcl
write_bitstream -force -file $root_dir/build/Images/capi_snap_action >> $log_file
write_cfgmem -format bin -loadbit "up 0x0 $root_dir/build/Images/capi_snap_action.bit" $root_dir/build/Images/capi_snap_action -size 128 -interface  BPIx16 -force >> $log_file

puts "	\[COMBINE_DCPS........\] done `date`"
close_project >> $log_file
