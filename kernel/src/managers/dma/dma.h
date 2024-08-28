// SPDX-FileCopyrightText: 2023 Ledger SAS
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIVATE_DEVICE_H
#define PRIVATE_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#define HANDLE_DMA 2UL

/**
 * NOTE: it could be interesting to use randstruct compiler plugin to
 * harden the handle corruption at runtime. Although, this may have a
 * small performance impact.
 */
typedef struct kdmah {
    uint32_t reserved: 21; /**< reserved part */
    uint32_t streamid: 8; /**< upto 256 streams */
    family: 3;            /**< handle type identifier */
} kdmah_t;

static_assert(sizeof(kdmah_t) == sizeof(uint32_t), "dma opaque sizeing failed!");

union udmah {
    const dmah_t *dh;
    const kdmah_t *kdh;
};

/**
 * NOTE: the union usage that allows a target memory to be multiple typed is not
 * FramaC compliant. To define if we aim to use a FramaC sprcific code for proof model
 * (meaning that these very API is out of the proof) or using a FramaC compliant API,
 * requiring a copy of the value instead of a local trans-typing
 * XXX: such usage is an UB in C++, meaning that it will be problematic in that case
 * these API are strictly dedicated to device internals and as such should not be included in
 * A C++ based test. A possibility is to add something like:
 * #ifdef __cplusplus
 * #error "Incompatible with C++ direct usage"
 * #endif
 */

 /**
  * @fn dmah_to_kdmah convert an opaque dma handle to a structured handle
  *
  * @param dh input dma handler
  *
  * @return converted handler pointer to structured value
  */
static inline const kdmah_t *dmah_to_kdmah(const dmah_t * const dh) {
    /*@ assert \valid(dh); */
    union udmah converter = {
        .dh = dh
    };
    return converter.kdh;
}

 /**
  * @fn kdmah_to_dmah convert a structured dma handler to an opaque handler
  *
  * @param kdh input structured dma handler
  *
  * @return converted handler pointer to opaque value
  */
static inline const dmah_t *kdmah_to_dmah(const kdmah_t * const kdh) {
    /*@ assert \valid(kdh); */
    union udmah converter = {
        .kdh = kdh
    };
    return converter.dh;
}

#ifdef __cplusplus
}
#endif

#endif/*!PRIVATE_DEVICE_H*/
