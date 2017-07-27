#!/bin/bash
export SNAP_TRACE=0xFF
$SNAP_ROOT/software/tools/snap_maint -v
$SNAP_ROOT/software/examples/snap_mc -o $SNAP_ROOT/hardware/sim/mc_output.txt -t 20000
