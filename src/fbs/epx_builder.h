#ifndef EPX_BUILDER_H
#define EPX_BUILDER_H

/* Generated by flatcc 0.5.3-pre FlatBuffers schema compiler for C by dvide.com */

#ifndef EPX_READER_H
#include "epx_reader.h"
#endif
#ifndef FLATBUFFERS_COMMON_BUILDER_H
#include "flatbuffers_common_builder.h"
#endif
#include "flatcc/flatcc_prologue.h"
#ifndef flatbuffers_identifier
#define flatbuffers_identifier 0
#endif
#ifndef flatbuffers_extension
#define flatbuffers_extension ".bin"
#endif

#define __epx_io_t_formal_args , epx_io_t_enum_t v0
#define __epx_io_t_call_args , v0
__flatbuffers_build_scalar(flatbuffers_, epx_io_t, epx_io_t_enum_t)
#define __epx_message_t_formal_args , epx_message_t_enum_t v0
#define __epx_message_t_call_args , v0
__flatbuffers_build_scalar(flatbuffers_, epx_message_t, epx_message_t_enum_t)
#define __epx_status_formal_args , epx_status_enum_t v0
#define __epx_status_call_args , v0
__flatbuffers_build_scalar(flatbuffers_, epx_status, epx_status_enum_t)

#define __epx_instance_id_formal_args , uint16_t v0, uint64_t v1
#define __epx_instance_id_call_args , v0, v1
static inline epx_instance_id_t *epx_instance_id_assign(epx_instance_id_t *p, uint16_t v0, uint64_t v1)
{ p->replica_id = v0; p->instance_id = v1;
  return p; }
static inline epx_instance_id_t *epx_instance_id_copy(epx_instance_id_t *p, const epx_instance_id_t *p2)
{ p->replica_id = p2->replica_id; p->instance_id = p2->instance_id;
  return p; }
static inline epx_instance_id_t *epx_instance_id_assign_to_pe(epx_instance_id_t *p, uint16_t v0, uint64_t v1)
{ flatbuffers_uint16_assign_to_pe(&p->replica_id, v0); flatbuffers_uint64_assign_to_pe(&p->instance_id, v1);
  return p; }
static inline epx_instance_id_t *epx_instance_id_copy_to_pe(epx_instance_id_t *p, const epx_instance_id_t *p2)
{ flatbuffers_uint16_copy_to_pe(&p->replica_id, &p2->replica_id); flatbuffers_uint64_copy_to_pe(&p->instance_id, &p2->instance_id);
  return p; }
static inline epx_instance_id_t *epx_instance_id_assign_from_pe(epx_instance_id_t *p, uint16_t v0, uint64_t v1)
{ flatbuffers_uint16_assign_from_pe(&p->replica_id, v0); flatbuffers_uint64_assign_from_pe(&p->instance_id, v1);
  return p; }
static inline epx_instance_id_t *epx_instance_id_copy_from_pe(epx_instance_id_t *p, const epx_instance_id_t *p2)
{ flatbuffers_uint16_copy_from_pe(&p->replica_id, &p2->replica_id); flatbuffers_uint64_copy_from_pe(&p->instance_id, &p2->instance_id);
  return p; }
__flatbuffers_build_struct(flatbuffers_, epx_instance_id, 16, 8, epx_instance_id_identifier, epx_instance_id_type_identifier)

#define __epx_dependency_formal_args , uint16_t v0, uint64_t v1, uint8_t v2
#define __epx_dependency_call_args , v0, v1, v2
static inline epx_dependency_t *epx_dependency_assign(epx_dependency_t *p, uint16_t v0, uint64_t v1, uint8_t v2)
{ p->replica_id = v0; p->instance_id = v1; p->committed = v2;
  return p; }
static inline epx_dependency_t *epx_dependency_copy(epx_dependency_t *p, const epx_dependency_t *p2)
{ p->replica_id = p2->replica_id; p->instance_id = p2->instance_id; p->committed = p2->committed;
  return p; }
static inline epx_dependency_t *epx_dependency_assign_to_pe(epx_dependency_t *p, uint16_t v0, uint64_t v1, uint8_t v2)
{ flatbuffers_uint16_assign_to_pe(&p->replica_id, v0); flatbuffers_uint64_assign_to_pe(&p->instance_id, v1); p->committed = v2;
  return p; }
static inline epx_dependency_t *epx_dependency_copy_to_pe(epx_dependency_t *p, const epx_dependency_t *p2)
{ flatbuffers_uint16_copy_to_pe(&p->replica_id, &p2->replica_id); flatbuffers_uint64_copy_to_pe(&p->instance_id, &p2->instance_id); p->committed = p2->committed;
  return p; }
static inline epx_dependency_t *epx_dependency_assign_from_pe(epx_dependency_t *p, uint16_t v0, uint64_t v1, uint8_t v2)
{ flatbuffers_uint16_assign_from_pe(&p->replica_id, v0); flatbuffers_uint64_assign_from_pe(&p->instance_id, v1); p->committed = v2;
  return p; }
static inline epx_dependency_t *epx_dependency_copy_from_pe(epx_dependency_t *p, const epx_dependency_t *p2)
{ flatbuffers_uint16_copy_from_pe(&p->replica_id, &p2->replica_id); flatbuffers_uint64_copy_from_pe(&p->instance_id, &p2->instance_id); p->committed = p2->committed;
  return p; }
__flatbuffers_build_struct(flatbuffers_, epx_dependency, 24, 8, epx_dependency_identifier, epx_dependency_type_identifier)

static const flatbuffers_voffset_t __epx_command_required[] = { 0 };
typedef flatbuffers_ref_t epx_command_ref_t;
static epx_command_ref_t epx_command_clone(flatbuffers_builder_t *B, epx_command_table_t t);
__flatbuffers_build_table(flatbuffers_, epx_command, 5)

static const flatbuffers_voffset_t __epx_instance_data_required[] = { 0 };
typedef flatbuffers_ref_t epx_instance_data_ref_t;
static epx_instance_data_ref_t epx_instance_data_clone(flatbuffers_builder_t *B, epx_instance_data_table_t t);
__flatbuffers_build_table(flatbuffers_, epx_instance_data, 3)

static const flatbuffers_voffset_t __epx_message_required[] = { 0 };
typedef flatbuffers_ref_t epx_message_ref_t;
static epx_message_ref_t epx_message_clone(flatbuffers_builder_t *B, epx_message_table_t t);
__flatbuffers_build_table(flatbuffers_, epx_message, 10)

static const flatbuffers_voffset_t __epx_batch_required[] = { 0 };
typedef flatbuffers_ref_t epx_batch_ref_t;
static epx_batch_ref_t epx_batch_clone(flatbuffers_builder_t *B, epx_batch_table_t t);
__flatbuffers_build_table(flatbuffers_, epx_batch, 2)

static const flatbuffers_voffset_t __epx_instance_required[] = { 0 };
typedef flatbuffers_ref_t epx_instance_ref_t;
static epx_instance_ref_t epx_instance_clone(flatbuffers_builder_t *B, epx_instance_table_t t);
__flatbuffers_build_table(flatbuffers_, epx_instance, 4)

#define __epx_command_formal_args ,\
  uint8_t v0, flatbuffers_string_ref_t v1, flatbuffers_string_ref_t v2, epx_io_t_enum_t v3, flatbuffers_uint8_vec_ref_t v4
#define __epx_command_call_args ,\
  v0, v1, v2, v3, v4
static inline epx_command_ref_t epx_command_create(flatbuffers_builder_t *B __epx_command_formal_args);
__flatbuffers_build_table_prolog(flatbuffers_, epx_command, epx_command_identifier, epx_command_type_identifier)

#define __epx_instance_data_formal_args , epx_command_ref_t v0, uint64_t v1, epx_dependency_vec_ref_t v2
#define __epx_instance_data_call_args , v0, v1, v2
static inline epx_instance_data_ref_t epx_instance_data_create(flatbuffers_builder_t *B __epx_instance_data_formal_args);
__flatbuffers_build_table_prolog(flatbuffers_, epx_instance_data, epx_instance_data_identifier, epx_instance_data_type_identifier)

#define __epx_message_formal_args ,\
  uint16_t v0, uint16_t v1, uint8_t v2, uint8_t v3,\
  epx_instance_id_t *v4, epx_message_t_enum_t v5, epx_instance_data_ref_t v6, epx_status_enum_t v7, uint64_t v8, uint64_t v9
#define __epx_message_call_args ,\
  v0, v1, v2, v3,\
  v4, v5, v6, v7, v8, v9
static inline epx_message_ref_t epx_message_create(flatbuffers_builder_t *B __epx_message_formal_args);
__flatbuffers_build_table_prolog(flatbuffers_, epx_message, epx_message_identifier, epx_message_type_identifier)

#define __epx_batch_formal_args , uint64_t v0, epx_message_vec_ref_t v1
#define __epx_batch_call_args , v0, v1
static inline epx_batch_ref_t epx_batch_create(flatbuffers_builder_t *B __epx_batch_formal_args);
__flatbuffers_build_table_prolog(flatbuffers_, epx_batch, epx_batch_identifier, epx_batch_type_identifier)

#define __epx_instance_formal_args , epx_instance_id_t *v0, uint8_t v1, epx_status_enum_t v2, epx_instance_data_ref_t v3
#define __epx_instance_call_args , v0, v1, v2, v3
static inline epx_instance_ref_t epx_instance_create(flatbuffers_builder_t *B __epx_instance_formal_args);
__flatbuffers_build_table_prolog(flatbuffers_, epx_instance, epx_instance_identifier, epx_instance_type_identifier)

__flatbuffers_build_scalar_field(0, flatbuffers_, epx_command_id, flatbuffers_uint8, uint8_t, 1, 1, UINT8_C(0), epx_command)
__flatbuffers_build_string_field(1, flatbuffers_, epx_command_start_key, epx_command)
__flatbuffers_build_string_field(2, flatbuffers_, epx_command_end_key, epx_command)
__flatbuffers_build_scalar_field(3, flatbuffers_, epx_command_writing, epx_io_t, epx_io_t_enum_t, 1, 1, UINT8_C(0), epx_command)
__flatbuffers_build_vector_field(4, flatbuffers_, epx_command_value, flatbuffers_uint8, uint8_t, epx_command)

static inline epx_command_ref_t epx_command_create(flatbuffers_builder_t *B __epx_command_formal_args)
{
    if (epx_command_start(B)
        || epx_command_start_key_add(B, v1)
        || epx_command_end_key_add(B, v2)
        || epx_command_value_add(B, v4)
        || epx_command_id_add(B, v0)
        || epx_command_writing_add(B, v3)) {
        return 0;
    }
    return epx_command_end(B);
}

static epx_command_ref_t epx_command_clone(flatbuffers_builder_t *B, epx_command_table_t t)
{
    __flatbuffers_memoize_begin(B, t);
    if (epx_command_start(B)
        || epx_command_start_key_pick(B, t)
        || epx_command_end_key_pick(B, t)
        || epx_command_value_pick(B, t)
        || epx_command_id_pick(B, t)
        || epx_command_writing_pick(B, t)) {
        return 0;
    }
    __flatbuffers_memoize_end(B, t, epx_command_end(B));
}

__flatbuffers_build_table_field(0, flatbuffers_, epx_instance_data_command, epx_command, epx_instance_data)
__flatbuffers_build_scalar_field(1, flatbuffers_, epx_instance_data_seq, flatbuffers_uint64, uint64_t, 8, 8, UINT64_C(0), epx_instance_data)
__flatbuffers_build_vector_field(2, flatbuffers_, epx_instance_data_deps, epx_dependency, epx_dependency_t, epx_instance_data)

static inline epx_instance_data_ref_t epx_instance_data_create(flatbuffers_builder_t *B __epx_instance_data_formal_args)
{
    if (epx_instance_data_start(B)
        || epx_instance_data_seq_add(B, v1)
        || epx_instance_data_command_add(B, v0)
        || epx_instance_data_deps_add(B, v2)) {
        return 0;
    }
    return epx_instance_data_end(B);
}

static epx_instance_data_ref_t epx_instance_data_clone(flatbuffers_builder_t *B, epx_instance_data_table_t t)
{
    __flatbuffers_memoize_begin(B, t);
    if (epx_instance_data_start(B)
        || epx_instance_data_seq_pick(B, t)
        || epx_instance_data_command_pick(B, t)
        || epx_instance_data_deps_pick(B, t)) {
        return 0;
    }
    __flatbuffers_memoize_end(B, t, epx_instance_data_end(B));
}

__flatbuffers_build_scalar_field(0, flatbuffers_, epx_message_from, flatbuffers_uint16, uint16_t, 2, 2, UINT16_C(0), epx_message)
__flatbuffers_build_scalar_field(1, flatbuffers_, epx_message_to, flatbuffers_uint16, uint16_t, 2, 2, UINT16_C(0), epx_message)
__flatbuffers_build_scalar_field(2, flatbuffers_, epx_message_nack, flatbuffers_uint8, uint8_t, 1, 1, UINT8_C(0), epx_message)
__flatbuffers_build_scalar_field(3, flatbuffers_, epx_message_ballot, flatbuffers_uint8, uint8_t, 1, 1, UINT8_C(0), epx_message)
__flatbuffers_build_struct_field(4, flatbuffers_, epx_message_instance_id, epx_instance_id, 16, 8, epx_message)
__flatbuffers_build_scalar_field(5, flatbuffers_, epx_message_type, epx_message_t, epx_message_t_enum_t, 1, 1, UINT8_C(0), epx_message)
__flatbuffers_build_table_field(6, flatbuffers_, epx_message_data, epx_instance_data, epx_message)
__flatbuffers_build_scalar_field(7, flatbuffers_, epx_message_instance_status, epx_status, epx_status_enum_t, 1, 1, UINT8_C(1), epx_message)
__flatbuffers_build_scalar_field(8, flatbuffers_, epx_message_srt, flatbuffers_uint64, uint64_t, 8, 8, UINT64_C(0), epx_message)
__flatbuffers_build_scalar_field(9, flatbuffers_, epx_message_stp, flatbuffers_uint64, uint64_t, 8, 8, UINT64_C(0), epx_message)

static inline epx_message_ref_t epx_message_create(flatbuffers_builder_t *B __epx_message_formal_args)
{
    if (epx_message_start(B)
        || epx_message_instance_id_add(B, v4)
        || epx_message_srt_add(B, v8)
        || epx_message_stp_add(B, v9)
        || epx_message_data_add(B, v6)
        || epx_message_from_add(B, v0)
        || epx_message_to_add(B, v1)
        || epx_message_nack_add(B, v2)
        || epx_message_ballot_add(B, v3)
        || epx_message_type_add(B, v5)
        || epx_message_instance_status_add(B, v7)) {
        return 0;
    }
    return epx_message_end(B);
}

static epx_message_ref_t epx_message_clone(flatbuffers_builder_t *B, epx_message_table_t t)
{
    __flatbuffers_memoize_begin(B, t);
    if (epx_message_start(B)
        || epx_message_instance_id_pick(B, t)
        || epx_message_srt_pick(B, t)
        || epx_message_stp_pick(B, t)
        || epx_message_data_pick(B, t)
        || epx_message_from_pick(B, t)
        || epx_message_to_pick(B, t)
        || epx_message_nack_pick(B, t)
        || epx_message_ballot_pick(B, t)
        || epx_message_type_pick(B, t)
        || epx_message_instance_status_pick(B, t)) {
        return 0;
    }
    __flatbuffers_memoize_end(B, t, epx_message_end(B));
}

__flatbuffers_build_scalar_field(0, flatbuffers_, epx_batch_id, flatbuffers_uint64, uint64_t, 8, 8, UINT64_C(0), epx_batch)
__flatbuffers_build_table_vector_field(1, flatbuffers_, epx_batch_messages, epx_message, epx_batch)

static inline epx_batch_ref_t epx_batch_create(flatbuffers_builder_t *B __epx_batch_formal_args)
{
    if (epx_batch_start(B)
        || epx_batch_id_add(B, v0)
        || epx_batch_messages_add(B, v1)) {
        return 0;
    }
    return epx_batch_end(B);
}

static epx_batch_ref_t epx_batch_clone(flatbuffers_builder_t *B, epx_batch_table_t t)
{
    __flatbuffers_memoize_begin(B, t);
    if (epx_batch_start(B)
        || epx_batch_id_pick(B, t)
        || epx_batch_messages_pick(B, t)) {
        return 0;
    }
    __flatbuffers_memoize_end(B, t, epx_batch_end(B));
}

__flatbuffers_build_struct_field(0, flatbuffers_, epx_instance_key, epx_instance_id, 16, 8, epx_instance)
__flatbuffers_build_scalar_field(1, flatbuffers_, epx_instance_ballot, flatbuffers_uint8, uint8_t, 1, 1, UINT8_C(0), epx_instance)
__flatbuffers_build_scalar_field(2, flatbuffers_, epx_instance_status, epx_status, epx_status_enum_t, 1, 1, UINT8_C(1), epx_instance)
__flatbuffers_build_table_field(3, flatbuffers_, epx_instance_idata, epx_instance_data, epx_instance)

static inline epx_instance_ref_t epx_instance_create(flatbuffers_builder_t *B __epx_instance_formal_args)
{
    if (epx_instance_start(B)
        || epx_instance_key_add(B, v0)
        || epx_instance_idata_add(B, v3)
        || epx_instance_ballot_add(B, v1)
        || epx_instance_status_add(B, v2)) {
        return 0;
    }
    return epx_instance_end(B);
}

static epx_instance_ref_t epx_instance_clone(flatbuffers_builder_t *B, epx_instance_table_t t)
{
    __flatbuffers_memoize_begin(B, t);
    if (epx_instance_start(B)
        || epx_instance_key_pick(B, t)
        || epx_instance_idata_pick(B, t)
        || epx_instance_ballot_pick(B, t)
        || epx_instance_status_pick(B, t)) {
        return 0;
    }
    __flatbuffers_memoize_end(B, t, epx_instance_end(B));
}

#include "flatcc/flatcc_epilogue.h"
#endif /* EPX_BUILDER_H */
