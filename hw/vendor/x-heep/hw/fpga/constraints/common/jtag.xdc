### JTAG Constraints
set_max_delay -through [get_nets -filter {NAME=~"*async*"} -of_objects [get_cells -hier -filter {REF_NAME =~ cdc_2phase_src* || ORIG_REF_NAME =~ cdc_2phase_src*}]] 20.000
set_false_path -hold -through [get_nets -filter {NAME=~"*async*"} -of_objects [get_cells -hier -filter {REF_NAME =~ cdc_2phase_src* || ORIG_REF_NAME =~ cdc_2phase_src*}]]

# Hold and max delay on 4 phases
#
# XDC files do not support Tcl control flow, so the foreach loop this replaces
# never had any effect; written as -of_objects commands like cdc_2phase_src above.
set_max_delay -through [get_nets -filter {NAME=~"*data_src_q*"} -of_objects [get_cells -hier -filter {REF_NAME == cdc_4phase_src || ORIG_REF_NAME == cdc_4phase_src}]] 20.000
set_false_path -hold -through [get_nets -filter {NAME=~"*data_src_q*"} -of_objects [get_cells -hier -filter {REF_NAME == cdc_4phase_src || ORIG_REF_NAME == cdc_4phase_src}]]
set_max_delay -through [get_nets -filter {NAME=~"*req_src_q*"} -of_objects [get_cells -hier -filter {REF_NAME == cdc_4phase_src || ORIG_REF_NAME == cdc_4phase_src}]] 20.000
set_false_path -hold -through [get_nets -filter {NAME=~"*req_src_q*"} -of_objects [get_cells -hier -filter {REF_NAME == cdc_4phase_src || ORIG_REF_NAME == cdc_4phase_src}]]

# cdc_reset_ctrlr (dmi_jtag's reset CDC) is a 2-flop synchronizer not covered by
# the cdc_2phase/cdc_4phase patterns above, and it fails single-cycle setup unless
# constrained. It is a hierarchical instance rather than a synthesized primitive,
# so it has no REF_NAME to filter on and is matched by pin name instead.
set_max_delay -through [get_pins -hier -filter {NAME =~ "*i_state_transition_cdc_src*_q_reg*"}] 20.000
set_false_path -hold -through [get_pins -hier -filter {NAME =~ "*i_state_transition_cdc_src*_q_reg*"}]
set_max_delay -through [get_pins -hier -filter {NAME =~ "*i_state_transition_cdc_dst*_q_reg*"}] 20.000
set_false_path -hold -through [get_pins -hier -filter {NAME =~ "*i_state_transition_cdc_dst*_q_reg*"}]
