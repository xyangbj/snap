#!/bin/bash
export SNAP_TRACE=0xFF
$SNAP_ROOT/software/tools/snap_maint -v
$SNAP_ROOT/actions/hls_mc/sw/snap_mc -o $SNAP_ROOT/actions/hls_mc/tests/mc_output.txt -t 20000
