#
# trap.tcl
#
#	Used in conjunction with trap.test.  This script traps
#	SIGINT and SIGQUIT
#
# RCS: $Header$

trap {
    puts "Caught SIGINT"
} SIGINT

trap {
    puts "Caught SIGQUIT"
} SIGQUIT

set x 1
vwait x
