#ifndef EPX_VERIFIER_H
#define EPX_VERIFIER_H

/* Generated by flatcc 0.5.3-pre FlatBuffers schema compiler for C by dvide.com */

#ifndef EPX_READER_H
#include "epx_reader.h"
#endif
#include "flatcc/flatcc_verifier.h"
#include "flatcc/flatcc_prologue.h"

static int epx_command_verify_table(flatcc_table_verifier_descriptor_t *td);
static int epx_instance_data_verify_table(flatcc_table_verifier_descriptor_t *td);
static int epx_message_verify_table(flatcc_table_verifier_descriptor_t *td);
static int epx_batch_verify_table(flatcc_table_verifier_descriptor_t *td);
static int epx_instance_verify_table(flatcc_table_verifier_descriptor_t *td);

static inline int epx_span_verify_as_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_struct_as_root(buf, bufsiz, epx_span_identifier, 16, 8);
}

static inline int epx_span_verify_as_typed_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_struct_as_typed_root(buf, bufsiz, epx_span_type_hash, 16, 8);
}

static inline int epx_span_verify_as_root_with_type_hash(const void *buf, size_t bufsiz, flatbuffers_thash_t thash)
{
    return flatcc_verify_struct_as_typed_root(buf, bufsiz, thash, 16, 8);
}

static inline int epx_span_verify_as_root_with_identifier(const void *buf, size_t bufsiz, const char *fid)
{
    return flatcc_verify_struct_as_root(buf, bufsiz, fid, 16, 8);
}

static inline int epx_instance_id_verify_as_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_struct_as_root(buf, bufsiz, epx_instance_id_identifier, 16, 8);
}

static inline int epx_instance_id_verify_as_typed_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_struct_as_typed_root(buf, bufsiz, epx_instance_id_type_hash, 16, 8);
}

static inline int epx_instance_id_verify_as_root_with_type_hash(const void *buf, size_t bufsiz, flatbuffers_thash_t thash)
{
    return flatcc_verify_struct_as_typed_root(buf, bufsiz, thash, 16, 8);
}

static inline int epx_instance_id_verify_as_root_with_identifier(const void *buf, size_t bufsiz, const char *fid)
{
    return flatcc_verify_struct_as_root(buf, bufsiz, fid, 16, 8);
}

static inline int epx_dependency_verify_as_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_struct_as_root(buf, bufsiz, epx_dependency_identifier, 24, 8);
}

static inline int epx_dependency_verify_as_typed_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_struct_as_typed_root(buf, bufsiz, epx_dependency_type_hash, 24, 8);
}

static inline int epx_dependency_verify_as_root_with_type_hash(const void *buf, size_t bufsiz, flatbuffers_thash_t thash)
{
    return flatcc_verify_struct_as_typed_root(buf, bufsiz, thash, 24, 8);
}

static inline int epx_dependency_verify_as_root_with_identifier(const void *buf, size_t bufsiz, const char *fid)
{
    return flatcc_verify_struct_as_root(buf, bufsiz, fid, 24, 8);
}

static int epx_command_verify_table(flatcc_table_verifier_descriptor_t *td)
{
    int ret;
    if ((ret = flatcc_verify_field(td, 0, 1, 1) /* id */)) return ret;
    if ((ret = flatcc_verify_field(td, 1, 16, 8) /* span */)) return ret;
    if ((ret = flatcc_verify_field(td, 2, 1, 1) /* writing */)) return ret;
    if ((ret = flatcc_verify_vector_field(td, 3, 0, 1, 1, INT64_C(4294967295)) /* value */)) return ret;
    return flatcc_verify_ok;
}

static inline int epx_command_verify_as_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_table_as_root(buf, bufsiz, epx_command_identifier, &epx_command_verify_table);
}

static inline int epx_command_verify_as_typed_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_table_as_root(buf, bufsiz, epx_command_type_identifier, &epx_command_verify_table);
}

static inline int epx_command_verify_as_root_with_identifier(const void *buf, size_t bufsiz, const char *fid)
{
    return flatcc_verify_table_as_root(buf, bufsiz, fid, &epx_command_verify_table);
}

static inline int epx_command_verify_as_root_with_type_hash(const void *buf, size_t bufsiz, flatbuffers_thash_t thash)
{
    return flatcc_verify_table_as_typed_root(buf, bufsiz, thash, &epx_command_verify_table);
}

static int epx_instance_data_verify_table(flatcc_table_verifier_descriptor_t *td)
{
    int ret;
    if ((ret = flatcc_verify_table_field(td, 0, 0, &epx_command_verify_table) /* command */)) return ret;
    if ((ret = flatcc_verify_field(td, 1, 8, 8) /* seq */)) return ret;
    if ((ret = flatcc_verify_vector_field(td, 2, 0, 24, 8, INT64_C(178956970)) /* deps */)) return ret;
    return flatcc_verify_ok;
}

static inline int epx_instance_data_verify_as_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_table_as_root(buf, bufsiz, epx_instance_data_identifier, &epx_instance_data_verify_table);
}

static inline int epx_instance_data_verify_as_typed_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_table_as_root(buf, bufsiz, epx_instance_data_type_identifier, &epx_instance_data_verify_table);
}

static inline int epx_instance_data_verify_as_root_with_identifier(const void *buf, size_t bufsiz, const char *fid)
{
    return flatcc_verify_table_as_root(buf, bufsiz, fid, &epx_instance_data_verify_table);
}

static inline int epx_instance_data_verify_as_root_with_type_hash(const void *buf, size_t bufsiz, flatbuffers_thash_t thash)
{
    return flatcc_verify_table_as_typed_root(buf, bufsiz, thash, &epx_instance_data_verify_table);
}

static int epx_message_verify_table(flatcc_table_verifier_descriptor_t *td)
{
    int ret;
    if ((ret = flatcc_verify_field(td, 0, 2, 2) /* to */)) return ret;
    if ((ret = flatcc_verify_field(td, 1, 1, 1) /* ballot */)) return ret;
    if ((ret = flatcc_verify_field(td, 2, 16, 8) /* instance_id */)) return ret;
    if ((ret = flatcc_verify_field(td, 3, 1, 1) /* type */)) return ret;
    if ((ret = flatcc_verify_table_field(td, 4, 0, &epx_instance_data_verify_table) /* data */)) return ret;
    return flatcc_verify_ok;
}

static inline int epx_message_verify_as_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_table_as_root(buf, bufsiz, epx_message_identifier, &epx_message_verify_table);
}

static inline int epx_message_verify_as_typed_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_table_as_root(buf, bufsiz, epx_message_type_identifier, &epx_message_verify_table);
}

static inline int epx_message_verify_as_root_with_identifier(const void *buf, size_t bufsiz, const char *fid)
{
    return flatcc_verify_table_as_root(buf, bufsiz, fid, &epx_message_verify_table);
}

static inline int epx_message_verify_as_root_with_type_hash(const void *buf, size_t bufsiz, flatbuffers_thash_t thash)
{
    return flatcc_verify_table_as_typed_root(buf, bufsiz, thash, &epx_message_verify_table);
}

static int epx_batch_verify_table(flatcc_table_verifier_descriptor_t *td)
{
    int ret;
    if ((ret = flatcc_verify_field(td, 0, 8, 8) /* id */)) return ret;
    if ((ret = flatcc_verify_table_vector_field(td, 1, 0, &epx_message_verify_table) /* messages */)) return ret;
    return flatcc_verify_ok;
}

static inline int epx_batch_verify_as_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_table_as_root(buf, bufsiz, epx_batch_identifier, &epx_batch_verify_table);
}

static inline int epx_batch_verify_as_typed_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_table_as_root(buf, bufsiz, epx_batch_type_identifier, &epx_batch_verify_table);
}

static inline int epx_batch_verify_as_root_with_identifier(const void *buf, size_t bufsiz, const char *fid)
{
    return flatcc_verify_table_as_root(buf, bufsiz, fid, &epx_batch_verify_table);
}

static inline int epx_batch_verify_as_root_with_type_hash(const void *buf, size_t bufsiz, flatbuffers_thash_t thash)
{
    return flatcc_verify_table_as_typed_root(buf, bufsiz, thash, &epx_batch_verify_table);
}

static int epx_instance_verify_table(flatcc_table_verifier_descriptor_t *td)
{
    int ret;
    if ((ret = flatcc_verify_field(td, 0, 16, 8) /* key */)) return ret;
    if ((ret = flatcc_verify_field(td, 1, 1, 1) /* ballot */)) return ret;
    if ((ret = flatcc_verify_field(td, 2, 1, 1) /* status */)) return ret;
    if ((ret = flatcc_verify_table_field(td, 3, 0, &epx_instance_data_verify_table) /* idata */)) return ret;
    return flatcc_verify_ok;
}

static inline int epx_instance_verify_as_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_table_as_root(buf, bufsiz, epx_instance_identifier, &epx_instance_verify_table);
}

static inline int epx_instance_verify_as_typed_root(const void *buf, size_t bufsiz)
{
    return flatcc_verify_table_as_root(buf, bufsiz, epx_instance_type_identifier, &epx_instance_verify_table);
}

static inline int epx_instance_verify_as_root_with_identifier(const void *buf, size_t bufsiz, const char *fid)
{
    return flatcc_verify_table_as_root(buf, bufsiz, fid, &epx_instance_verify_table);
}

static inline int epx_instance_verify_as_root_with_type_hash(const void *buf, size_t bufsiz, flatbuffers_thash_t thash)
{
    return flatcc_verify_table_as_typed_root(buf, bufsiz, thash, &epx_instance_verify_table);
}

#include "flatcc/flatcc_epilogue.h"
#endif /* EPX_VERIFIER_H */
