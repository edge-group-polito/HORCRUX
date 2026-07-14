// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: crheepto.h
// Author: Luigi Giuffrida, Valeria Piscopo
// Date: 09/01/2026
// Description: Address map for crheepto external peripherals.

#ifndef CRHEEPTO_H
#define CRHEEPTO_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "core_v_mini_mcu.h"

// Number of masters and slaves on the external crossbar
#define EXT_XBAR_NMASTER ${xbar_nmasters}
#define EXT_XBAR_NSLAVE ${xbar_nslaves}


// Memory map
// ----------

% if (xbar_nslaves > 0):

% for a_slave in slaves:

// ${a_slave['SCREAMING_NAME']}
#define ${a_slave['SCREAMING_NAME']}_START_ADDRESS EXT_SLAVE_START_ADDRESS + 0x${a_slave['offset']}
#define ${a_slave['SCREAMING_NAME']}_SIZE 0x${a_slave['size']}
#define ${a_slave['SCREAMING_NAME']}_END_ADDRESS ${a_slave['SCREAMING_NAME']}_START_ADDRESS + 0x${a_slave['size']}
% endfor

% endif

// Peripheral map
// ----------

% if (periph_nslaves > 0):

% for a_slave in peripherals:

// ${a_slave['SCREAMING_NAME']}
#define ${a_slave['SCREAMING_NAME']}_PERIPH_START_ADDRESS EXT_PERIPHERAL_START_ADDRESS + 0x${a_slave['offset']}
#define ${a_slave['SCREAMING_NAME']}_PERIPH_SIZE 0x${a_slave['size']}
#define ${a_slave['SCREAMING_NAME']}_PERIPH_END_ADDRESS ${a_slave['SCREAMING_NAME']}_PERIPH_START_ADDRESS + 0x${a_slave['size']}
% endfor

% endif

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // CRHEEPTO_H
