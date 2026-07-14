#include <stdint.h>
#include "params.h"
#include "reduce.h"

/* Note: SW implementations of montgomery_reduce and barrett_reduce are
 * not needed - all usages replaced by HW instructions:
 * - OP_MQMULK for Montgomery multiplication (fqmul, basemul, poly_tomont)
 * - OP_BARRETT for Barrett reduction (poly_reduce)
 */
