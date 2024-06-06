#!/bin/sh

DEBUGFS=`grep debugfs /proc/mounts | awk '{ print $2; }'`
echo $$ > $DEBUGFS/tracing/set_ftrace_pid
echo nop > $DEBUGFS/tracing/current_tracer
exec $*
