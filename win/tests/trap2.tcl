#
# trap2.tcl
#
#	Used in conjunction with trap.test.  This script traps
#	SIGINT and SIGQUIT.
#
# RCS: $Header$

set exp_nt_debug 1
spawn telnet quimby.cs.berkeley.edu

trap {
    puts "Caught SIGINT"
} SIGINT

trap {
    puts "Caught SIGQUIT"
    close
} SIGQUIT

set x 1
vwait x
