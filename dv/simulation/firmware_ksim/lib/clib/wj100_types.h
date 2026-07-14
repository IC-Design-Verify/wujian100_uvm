/*
 * wujian100_open Common Type Definitions for register headers
 * Generated from wujian100_open Userguide v1.0
 *
 * Header files in the firmware distribution typically include datatype.h,
 * which provides uint32_t, reg32_t, etc. This shim provides the same type
 * in environments where datatype.h is unavailable (e.g. host compilation).
 */
#ifndef __WJ100_TYPES_H__
#define __WJ100_TYPES_H__

#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 199901L
#include <stdint.h>
#define _WJ100_HAS_STDINT 1
#endif
#endif

#ifndef _WJ100_HAS_STDINT
/* Fallback when stdint.h is not available (rare embedded toolchains that
 * only expose datatype.h from firmware_ksim/lib/clib/). */
#ifndef _WJ100_UINT32_TYPEDEFED
#define _WJ100_UINT32_TYPEDEFED
typedef unsigned int uint32_t;
#endif
#endif

#endif /* __WJ100_TYPES_H__ */
