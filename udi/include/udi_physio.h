/* Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish,
 * use, compile, sell and distribute this work
 * and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions,
 * without complying with any conditions
 * and by any means.
 */

#if !defined (UDI_PHYSIO_VERSION)
 #error "UDI_PHYSIO_VERSION must be defined."
#elif (UDI_PHYSIO_VERSION != 0x101)
 #error "Unsupported UDI_PHYSIO_VERSION."
#endif

#define NULL ((void *)0)
#define _UDI_HANDLE void *
#define _UDI_NULL_HANDLE NULL

typedef _UDI_HANDLE udi_dma_constraints_t;
#define UDI_NULL_DMA_CONSTRAINTS _UDI_NULL_HANDLE

typedef udi_ubit8_t udi_dma_constraints_attr_t;
#define UDI_DMA_ADDRESSABLE_BITS         100
#define UDI_DMA_ALIGNMENT_BITS           101

#define UDI_DMA_DATA_ADDRESSABLE_BITS    110
#define UDI_DMA_NO_PARTIAL               111

#define UDI_DMA_SCGTH_MAX_ELEMENTS       120
#define UDI_DMA_SCGTH_FORMAT             121
#define UDI_DMA_SCGTH_ENDIANNESS         122
#define UDI_DMA_SCGTH_ADDRESSABLE_BITS   123
#define UDI_DMA_SCGTH_MAX_SEGMENTS       124

#define UDI_DMA_SCGTH_ALIGNMENT_BITS     130
#define UDI_DMA_SCGTH_MAX_EL_PER_SEG     131
#define UDI_DMA_SCGTH_PREFIX_BYTES       132

#define UDI_DMA_ELEMENT_ALIGNMENT_BITS   140
#define UDI_DMA_ELEMENT_LENGTH_BITS      141
#define UDI_DMA_ELEMENT_GRANULARITY_BITS 142

#define UDI_DMA_ADDR_FIXED_BITS          150
#define UDI_DMA_ADDR_FIXED_TYPE          151
#define UDI_DMA_ADDR_FIXED_VALUE_LO      152
#define UDI_DMA_ADDR_FIXED_VALUE_HI      153

#define UDI_DMA_SEQUENTIAL               160
#define UDI_DMA_SLOP_IN_BITS             161
#define UDI_DMA_SLOP_OUT_BITS            162
#define UDI_DMA_SLOP_OUT_EXTRA           163
#define UDI_DMA_SLOP_BARRIER_BITS        164

#define UDI_DMA_LITTLE_ENDIAN            (1U << 6)
#define UDI_DMA_BIG_ENDIAN               (1U << 5)

#define UDI_DMA_FIXED_ELEMENT            1
#define UDI_DMA_FIXED_LIST               2
#define UDI_DMA_FIXED_VALUE              3

typedef struct {
    udi_dma_constraints_attr_t attr_type;
    udi_ubit32_t attr_value;
} udi_dma_constraints_attr_spec_t;

typedef void udi_dma_constraints_attr_set_call_t(
    udi_cb_t *gcb,
    udi_dma_constraints_t new_constraints,
    udi_status_t status);

void udi_dma_constraints_attr_set(
    udi_dma_constraints_attr_set_call_t *callback,
    udi_cb_t *gcb,
    udi_dma_constraints_t src_constraints,
    const udi_dma_constraints_attr_spec_t *attr_list,
    udi_ubit16_t list_length,
    udi_ubit8_t flags);

#define UDI_DMA_CONSTRAINTS_COPY (1U << 0)

void udi_dma_constraints_attr_reset(udi_dma_constraints_t constraints,
                                    udi_dma_constraints_attr_t attr_type);

void udi_dma_constraints_free(udi_dma_constraints_t constraints);

typedef _UDI_HANDLE udi_dma_handle_t;
#define UDI_NULL_DMA_HANDLE _UDI_NULL_HANDLE

typedef struct {
    udi_size_t max_legal_contig_alloc;
    udi_size_t max_safe_contig_alloc;
    udi_size_t cache_line_size;
} udi_dma_limits_t;

#define UDI_DMA_MIN_ALLOC_LIMIT 4000

void udi_dma_limits(udi_dma_limits_t *dma_limits);

typedef struct {
    /* OPAQUE */
    udi_ubit8_t addr0;
    udi_ubit8_t addr1;
    udi_ubit8_t addr2;
    udi_ubit8_t addr3;
    udi_ubit8_t addr4;
    udi_ubit8_t addr5;
    udi_ubit8_t addr6;
    udi_ubit8_t addr7;
} udi_busaddr64_t;

typedef struct {
    udi_ubit32_t block_busaddr;
    udi_ubit32_t block_length;
} udi_scgth_element_32_t;

typedef struct {
    udi_busaddr64_t block_busaddr;
    udi_ubit32_t block_length;
    udi_ubit32_t el_reserved;
} udi_scgth_element_64_t;

#define UDI_SCGTH_EXT 0x80000000

typedef struct {
    udi_ubit16_t scgth_num_elements;
    udi_ubit8_t scgth_format;
    udi_boolean_t scgth_must_swap;
    union {
        udi_scgth_element_32_t *el32p;
        udi_scgth_element_64_t *el64p;
    } scgth_elements;
    union {
        udi_scgth_element_32_t el32;
        udi_scgth_element_64_t el64;
    } scgth_first_element;
} udi_scgth_t;
/* Values for scgth_format: */
#define UDI_SCGTH_32            (1U << 0)
#define UDI_SCGTH_64            (1U << 1)
#define UDI_SCGTH_DMA_MAPPED    (1U << 6)
#define UDI_SCGTH_DRIVER_MAPPED (1U << 7)


typedef void udi_dma_prepare_call_t(udi_cb_t *gcb,
                                    udi_dma_handle_t *new_dma_handle);
void udi_dma_prepare(udi_dma_prepare_call_t *callback,
                     udi_cb_t *gcb,
                     udi_dma_constraints_t constraints,
                     udi_ubit8_t flags);

typedef void udi_dma_buf_map_call_t(udi_cb_t *gcb,
                                    udi_scgth_t *scgth,
                                    udi_boolean_t complete,
                                    udi_status_t status);
void udi_dma_buf_map(udi_dma_buf_map_call_t *callback,
                     udi_cb_t *gcb,
                     udi_dma_handle_t dma_handle,
                     udi_buf_t *buf,
                     udi_size_t offset,
                     udi_size_t len,
                     udi_ubit8_t flags);

udi_buf_t *udi_dma_buf_unmap(udi_dma_handle_t dma_handle,
                             udi_size_t new_buf_size);

typedef void udi_dma_mem_alloc_call_t(udi_cb_t *gcb,
                                      udi_dma_handle_t new_dma_handle,
                                      void *mem_ptr,
                                      udi_size_t actual_gap,
                                      udi_boolean_t single_element,
                                      udi_scgth_t *scgth,
                                      udi_boolean_t must_swap);
void udi_dma_mem_alloc(udi_dma_mem_alloc_call_t *callback,
                       udi_cb_t *gcb,
                       udi_dma_constraints_t constraints,
                       udi_ubit8_t flags,
                       udi_ubit16_t nelements,
                       udi_size_t element_size,
                       udi_size_t max_gap);

typedef void udi_dma_sync_call_t(udi_cb_t *gcb);
void udi_dma_sync(udi_dma_sync_call_t *callback,
                  udi_cb_t *gcb,
                  udi_dma_handle_t dma_handle,
                  udi_size_t offset,
                  udi_size_t length,
                  udi_ubit8_t flags);

/* flags: */
#define UDI_DMA_OUT           (1U << 2)
#define UDI_DMA_IN            (1U << 3)
#define UDI_DMA_REWIND        (1U << 4)
#define UDI_DMA_BIG_ENDIAN    (1U << 5)
#define UDI_DMA_LITTLE_ENDIAN (1U << 6)
#define UDI_DMA_NEVER_SWAP    (1U << 7)

typedef void udi_dma_scgth_sync_call_t(udi_cb_t *gcb);
void udi_dma_scgth_sync(udi_dma_scgth_sync_call_t *callback,
                        udi_cb_t *gcb,
                        udi_dma_handle_t dma_handle);

void udi_dma_mem_barrier(udi_dma_handle_t dma_handle);

void udi_dma_free(udi_dma_handle_t dma_handle);

typedef void udi_dma_mem_to_buf_call_t(udi_cb_t *gcb,
                                       udi_buf_t *new_dst_buf);
void udi_dma_mem_to_buf(udi_dma_mem_to_buf_call_t *callback,
                        udi_cb_t *gcb,
                        udi_dma_handle_t dma_handle,
                        udi_size_t src_off,
                        udi_size_t src_len,
                        udi_buf_t *dst_buf);

/* DMA constraints handle layout element type code: */
#define UDI_DL_DMA_CONSTRAINTS_T 201



typedef _UDI_HANDLE udi_pio_handle_t;
#define UDI_NULL_PIO_HANDLE _UDI_NULL_HANDLE

typedef const struct {
    udi_ubit8_t pio_op;
    udi_ubit8_t tran_size;
    udi_ubit16_t operand;
} udi_pio_trans_t;
/* tran_size: */
#define UDI_PIO_1BYTE  0
#define UDI_PIO_2BYTE  1
#define UDI_PIO_4BYTE  2
#define UDI_PIO_8BYTE  3
#define UDI_PIO_16BYTE 4
#define UDI_PIO_32BYTE 5

/* Register numbers in pio_op: */
#define UDI_PIO_R0 0
#define UDI_PIO_R1 1
#define UDI_PIO_R2 2
#define UDI_PIO_R3 3
#define UDI_PIO_R4 4
#define UDI_PIO_R5 5
#define UDI_PIO_R6 6
#define UDI_PIO_R7 7

/* Addressing modes in pio_op: */
#define UDI_PIO_DIRECT  0x00
#define UDI_PIO_SCRATCH 0x08
#define UDI_PIO_BUF     0x10
#define UDI_PIO_MEM     0x18

/* Class A opcodes in pio_op: */
#define UDI_PIO_IN    0x00
#define UDI_PIO_OUT   0x20
#define UDI_PIO_LOAD  0x40
#define UDI_PIO_STORE 0x60

/* Class B opcodes in pio_op: */
#define UDI_PIO_LOAD_IMM    0x80
#define UDI_PIO_CSKIP       0x88
#define UDI_PIO_IN_IND      0x90
#define UDI_PIO_OUT_IND     0x98
#define UDI_PIO_SHIFT_LEFT  0xA0
#define UDI_PIO_SHIFT_RIGHT 0xA8
#define UDI_PIO_AND         0xB0
#define UDI_PIO_AND_IMM     0xB8
#define UDI_PIO_OR          0xC0
#define UDI_PIO_OR_IMM      0xC8
#define UDI_PIO_XOR         0xD0
#define UDI_PIO_ADD         0xD8
#define UDI_PIO_ADD_IMM     0xE0
#define UDI_PIO_SUB         0xE8

/* Class C opcodes in pio_op: */
#define UDI_PIO_BRANCH      0xF0
#define UDI_PIO_LABEL       0xF1
#define UDI_PIO_REP_IN_IND  0xF2
#define UDI_PIO_REP_OUT_IND 0xF3
#define UDI_PIO_DELAY       0xF4
#define UDI_PIO_BARRIER     0xF5
#define UDI_PIO_SYNC        0xF6
#define UDI_PIO_SYNC_OUT    0xF7
#define UDI_PIO_DEBUG       0xF8
#define UDI_PIO_END         0xFE
#define UDI_PIO_END_IMM     0xFF

/* UDI_PIO_DEBUG operands: */
#define UDI_PIO_TRACE_OPS_NONE  0
#define UDI_PIO_TRACE_OPS1      1
#define UDI_PIO_TRACE_OPS2      2
#define UDI_PIO_TRACE_OPS3      3
#define UDI_PIO_TRACE_REGS_NONE (0U << 2)
#define UDI_PIO_TRACE_REGS1     (1U << 2)
#define UDI_PIO_TRACE_REGS2     (2U << 2)
#define UDI_PIO_TRACE_REGS3     (3U << 2)
#define UDI_PIO_TRACE_DEV_NONE  (0U << 4)
#define UDI_PIO_TRACE_DEV1      (1U << 4)
#define UDI_PIO_TRACE_DEV2      (2U << 4)
#define UDI_PIO_TRACE_DEV3      (3U << 4)

/* PIO condition codes: */
#define UDI_PIO_Z    0
#define UDI_PIO_NZ   1
#define UDI_PIO_NEG  2
#define UDI_PIO_NNEG 3

#define UDI_PIO_REP_ARGS\
 (mode, mem_reg, mem_stride, pio_reg, pio_stride, cnt_reg) \
 ((mode) | (mem_reg) | ((mem_stride) << 5) \
  | ((pio_reg) << 7) | ((pio_stride) << 10) | ((cnt_reg) << 13))


typedef void udi_pio_map_call_t(udi_cb_t *gcb,
                                udi_pio_handle_t new_pio_handle);
void udi_pio_map(udi_pio_map_call_t *callback,
                 udi_cb_t *gcb,
                 udi_ubit32_t regset_idx,
                 udi_ubit32_t base_offset,
                 udi_ubit32_t length,
                 udi_pio_trans_t *trans_list,
                 udi_ubit16_t list_length,
                 udi_ubit16_t pio_attributes,
                 udi_ubit32_t pace,
                 udi_index_t serialization_domain);
/* pio_attributes: */
#define UDI_PIO_STRICTORDER     (1U << 0)
#define UDI_PIO_UNORDERED_OK    (1U << 1)
#define UDI_PIO_MERGING_OK      (1U << 2)
#define UDI_PIO_LOADCACHING_OK  (1U << 3)
#define UDI_PIO_STORECACHING_OK (1U << 4)
#define UDI_PIO_BIG_ENDIAN      (1U << 5)
#define UDI_PIO_LITTLE_ENDIAN   (1U << 6)
#define UDI_PIO_NEVERSWAP       (1U << 7)
#define UDI_PIO_UNALIGNED       (1U << 8)

void udi_pio_unmap(udi_pio_handle_t pio_handle);

udi_ubit32_t udi_pio_atomic_sizes(udi_pio_handle_t pio_handle);

void udi_pio_abort_sequence(udi_pio_handle_t pio_handle,
                            udi_size_t scratch_requirement);

typedef void udi_pio_trans_call_t(udi_cb_t *gcb,
                                  udi_buf_t *new_buf,
                                  udi_status_t status,
                                  udi_ubit16_t result);
void udi_pio_trans(udi_pio_trans_call_t *callback,
                   udi_cb_t *gcb,
                   udi_pio_handle_t pio_handle,
                   udi_index_t start_label,
                   udi_buf_t *buf,
                   void *mem_ptr);

typedef void udi_pio_probe_call_t(udi_cb_t *gcb,
                                  udi_status_t status);
void udi_pio_probe(udi_pio_probe_call_t *callback,
                   udi_cb_t *gcb,
                   udi_pio_handle_t pio_handle,
                   void *mem_ptr,
                   udi_ubit32_t pio_offset,
                   udi_ubit8_t tran_size,
                   udi_ubit8_t direction);
/* direction: */
#define UDI_PIO_IN  0x00
#define UDI_PIO_OUT 0x20

/* PIO handle layout element type code: */
#define UDI_DL_PIO_HANDLE_T 200



/* Flag values for interrupt handling: */
#define UDI_INTR_UNCLAIMED (1U << 0)
#define UDI_INTR_NO_EVENT  (1U << 1)

typedef struct {
    udi_cb_t gcb;
} udi_bus_bind_cb_t;

typedef struct {
    udi_cb_t gcb;
    udi_index_t interrupt_idx;
    udi_ubit8_t min_event_pend;
    udi_pio_handle_t preprocessing_handle;
} udi_intr_attach_cb_t;

typedef struct {
    udi_cb_t gcb;
    udi_index_t interrupt_idx;
} udi_intr_detach_cb_t;

typedef struct {
    udi_cb_t gcb;
    udi_buf_t *event_buf;
    udi_ubit16_t intr_result;
} udi_intr_event_cb_t;

#define UDI_BUS_BIND_CB_NUM        1
#define UDI_BUS_INTR_ATTACH_CB_NUM 2
#define UDI_BUS_INTR_DETACH_CB_NUM 3
#define UDI_BUS_INTR_EVENT_CB_NUM  4

void udi_bus_bind_req(udi_bus_bind_cb_t *cb);
void udi_bus_bind_ack(udi_bus_bind_cb_t *cb,
                      udi_dma_constraints_t dma_constraints,
                      udi_ubit8_t preferred_endianess,
                      udi_status_t status);
void udi_bus_unbind_req(udi_bus_bind_cb_t *cb);
void udi_bus_unbind_ack(udi_bus_bind_cb_t *cb);
void udi_intr_attach_req(udi_intr_attach_cb_t *intr_attach_cb);
void udi_intr_attach_ack(udi_intr_attach_cb_t *intr_attach_cb,
                         udi_status_t status);
void udi_intr_detach_req(udi_intr_detach_cb_t *intr_detach_cb);
void udi_intr_detach_ack(udi_intr_detach_cb_t *intr_detach_cb);
void udi_intr_event_ind(udi_intr_event_cb_t *intr_event_cb,
                        udi_ubit8_t flags);
void udi_intr_event_rdy(udi_intr_event_cb_t *intr_event_cb);

typedef void udi_bus_bind_req_op_t(udi_bus_bind_cb_t *gcb);
typedef void udi_bus_bind_ack_op_t(udi_bus_bind_cb_t *cb,
                                   udi_dma_constraints_t dma_constraints,
                                   udi_ubit8_t preferred_endianness,
                                   udi_status_t status);
typedef void udi_bus_unbind_req_op_t(udi_bus_bind_cb_t *cb);
typedef void udi_bus_unbind_ack_op_t(udi_bus_bind_cb_t *cb);
typedef void udi_intr_attach_req_op_t(udi_intr_attach_cb_t *intr_attach_cb);
typedef void udi_intr_attach_ack_op_t(udi_intr_attach_cb_t *intr_attach_cb,
                                      udi_status_t status);
typedef void udi_intr_detach_req_op_t(udi_intr_detach_cb_t *intr_detach_cb);
typedef void udi_intr_detach_ack_op_t(udi_intr_detach_cb_t *intr_detach_cb);
typedef void udi_intr_event_ind_op_t(udi_intr_event_cb_t *intr_event_cb,
                                     udi_ubit8_t flags);
typedef void udi_intr_event_rdy_op_t(udi_intr_event_cb_t *intr_event_cb);

/* preferred_endianness: */
#define UDI_DMA_BIG_ENDIAN    (1U << 5)
#define UDI_DMA_LITTLE_ENDIAN (1U << 6)
#define UDI_DMA_ANY_ENDIAN    (1U << 0)

/* flags: */
#define UDI_INTR_MASKING_NOT_REQUIRED (1U << 0)
#define UDI_INTR_OVERRUN_OCCURRED     (1U << 1)
#define UDI_INTR_PREPROCESSED         (1U << 2)

typedef const struct {
    udi_channel_event_ind_op_t *channel_event_ind_op;
    udi_bus_bind_ack_op_t *bus_bind_ack_op;
    udi_bus_unbind_ack_op_t *bus_unbind_ack_op;
    udi_intr_attach_ack_op_t *intr_attach_ack_op;
    udi_intr_detach_ack_op_t *intr_detach_ack_op;
} udi_bus_device_ops_t;

typedef const struct {
    udi_channel_event_ind_op_t *channel_event_ind_op;
    udi_bus_bind_req_op_t *bus_bind_req_op;
    udi_bus_unbind_req_op_t *bus_unbind_req_op;
    udi_intr_attach_req_op_t *intr_attach_req_op;
    udi_intr_detach_req_op_t *intr_detach_req_op;
} udi_bus_bridge_ops_t;

typedef const struct {
    udi_channel_event_ind_op_t *channel_event_ind_op;
    udi_intr_event_ind_op_t *intr_event_ind_op;
} udi_intr_handle_ops_t;

typedef const struct {
    udi_channel_event_ind_op_t *channel_event_ind_op;
    udi_intr_event_rdy_op_t *intr_event_rdy_op;
} udi_intr_dispatcher_ops_t;

#define UDI_BUS_DEVICE_OPS_NUM        1
#define UDI_BUS_BRIDGE_OPS_NUM        2
#define UDI_BUS_INTR_HANDLER_OPS_NUM  3
#define UDI_BUS_INTR_DISPATCH_OPS_NUM 4

/* Proxies: */
udi_intr_attach_ack_op_t udi_intr_attach_ack_unused;
udi_intr_detach_ack_op_t udi_intr_detach_ack_unused;
