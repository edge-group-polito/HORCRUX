#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"

#define MONT -1044 // 2^16 mod q
#define QINV -3327 // q^-1 mod 2^16

/* Note: SW implementations removed - all usages replaced by HW instructions:
 * - OP_MQMULK for Montgomery multiplication
 * - OP_BARRETT for Barrett reduction
 */

#endif
