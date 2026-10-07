### synch cells Constraints
#
# XDC files do not support Tcl control flow, so the foreach loop this replaces
# never had any effect. REF_NAME=="sync" does not match post-synthesis either,
# because Vivado mangles parameterized module names; match the instance name
# "i_sync" through a hierarchical pin search instead.
set_max_delay -through [get_pins -hier -filter {NAME=~"*/i_sync/reg_q_reg[0]/D"}] 20.000
set_false_path -hold -through [get_pins -hier -filter {NAME=~"*/i_sync/reg_q_reg[0]/D"}]

