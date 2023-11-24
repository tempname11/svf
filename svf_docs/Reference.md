# Reference

This document describes the current [C](#c) and [C++](#c-cpp) API. The C++ API is a bit nicer,
and safeguards against some mistakes, because it uses templates, and not macros.
Both are pretty similar otherwise.

No function ever allocates memory, with the notable exception of `read_message`,
which may call the user-provided allocator exactly once.

## C++ (cpp)

Intended to be used with C++11 and higher. All functions are inside the `svf::runtime` namespace. The API also does not currently use the C++ `std` library, or RAII style, and is mostly intended as a nicer C API.

Exceptions are never used. These structures have error codes:

- `ReadMessageResult`: `error_code` can be set after `read_message`.
- `WriteContext`: `error_code` may be set after any of the writing functions. An error does not prevent further write calls, however, they will do nothing.

See `SVFRT_ErrorCode` in the headers to see all possible codes.

---

- [`read_message`](#read_message)
- [`read_reference`](#read_reference)
- [`read_sequence_element`](#read_sequence_element)
- [`read_sequence_raw`](#read_sequence_raw)

---

- [`write_start`](#write_start)
- [`write_reference`](#write_reference)
- [`write_sequence`](#write_sequence)
- [`write_fixed_size_array`](#write_fixed_size_array)
- [`write_fixed_size_string`](#write_fixed_size_string)
- [`write_sequence_element`](#write_sequence_element)
- [`write_finish`](#write_finish)

---

### `read_message`
```cpp
template<typename Entry>
ReadMessageResult<Entry> read_message(
  Range<uint8_t> message,
  Range<uint8_t> scratch,
  CompatibilityLevel required_level,
  AllocatorFn *allocator_fn = NULL,
  void *allocator_ptr = NULL,
  SchemaLookupFn *schema_lookup_fn = NULL,
  void *schema_lookup_ptr = NULL
) noexcept;
```
Read a message according to an expected schema. May fail, returning an error code. The expected schema will depend on the template parameter type; intended usage is just specifying one of the structures in the generated header file.

Some additional parameters are currently not exposed by this function. See `SVFRT_read_message` if more precise control is needed.

Parameters:
- `message`: the whole SVF message, as a contiguous buffer.
- `scratch`: a buffer with scratch memory. See `min_read_scratch_memory_size` in the generated header file of your schema. Typically should be allocated on the stack (as the size is known at compile time).
- `required_level`: the required compatibility level between the expected schema, and the actual schema (one of: logical, binary, exact). `read_message` will fail, if the resulting compatibility is weaker than required.
- `allocator_fn`: an allocator function, necessary for logical compatibility, for which an allocation might be needed. `read_message` will call this function at most once.
- `allocator_ptr`: will be passed as the first argument to `allocator_fn`.
- `schema_lookup_fn`: when using incomplete messages with the schema omitted, this should be provided to look up a schema from its hash. The message will be interpreted according to the resulting schema. `read_message` will call this function at most once.
- `schema_lookup_ptr`: will be passed as the first argument to `schema_lookup_fn`.

Returns a structure with:
- `error_code`: 0 on success, or a specific error code (see `SVFRT_code_read_*`, `SVFRT_code_compatiblity_*`, `SVFRT_code_conversion_*` in the headers).
- `entry`: a pointer to the entry structure. Will be `nullptr` on read failure.
- `allocation`: if `allocator_fn` was provided, this may be a valid pointer. **Important**: cleanup for this pointer (typically, `free`, or C++ `delete`) after all reading operations are finished, is up to the user.
- `compatibility_level`: the resulting compatibility level between the expected schema, and the actual schema (one of: none, logical, binary, exact).
- `context`: a structure that should be used for subsequent reading operations.

### `read_reference`
```cpp
template<typename T>
T const *read_reference(
  ReadContext *ctx,
  Reference<T> reference
) noexcept;
```

Access a reference.

Parameters:
- `ctx`: a pointer to the read context (see `read_message`).
- `reference`: the reference to read.

Returns a valid pointer, if the reference is valid, or `nullptr` otherwise.

### `read_sequence_element`
```cpp
template<typename T>
T const *read_sequence_element(
  ReadContext *ctx,
  Sequence<T> sequence,
  uint32_t element_index
) noexcept;
```

Access a single element from a sequence.

Parameters:
- `ctx`: a pointer to the read context (see `read_message`).
- `sequence`: the sequence to read.
- `element_index`: the index of the element to read.

Returns a valid pointer, if the sequence is valid and contains the index, or `nullptr` otherwise.

### `read_sequence_raw`
```cpp
template<typename T>
Range<T const> read_sequence_raw(
  ReadContext *ctx,
  Sequence<T> sequence
) noexcept;
```

Access all elements from a sequence. This can only be applied to a sequence of
primitives (integers or floats) because in the general case, with binary
compatibility, the actual structure stride may be different from the expected
structure stride and C-style array indexing no longer applies. Use `read_sequence_element` for structures.

Parameters:
- `ctx`: a pointer to the read context (see `read_message`).
- `sequence`: the sequence to read.

Returns a valid range, if the sequence is valid, or `nullptr` otherwise.

### `write_start`
```cpp
template<typename Entry>
WriteContext<Entry> write_start(
  WriterFn *writer_fn,
  void *writer_ptr
) noexcept;
```

Start writing a message. Will write the header and schema, and prepare everything for further operations.

Parameters:
- `writer_fn`: a function which will actually write designated bytes to the output, be it a file, buffer, or otherwise. Should return the number of bytes actually written, or 0 on error. May be called an arbitrary number of times during the subsequent writing operations.
- `writer_ptr`: a pointer to be passed to `writer_fn`.

Returns the write context, used for all subsequent operations. The `error_code` inside the context may be set on an error (see `SVFRT_code_write_*` in the headers), either immediately, or after subsequent writing operations.

### `write_reference`
```cpp
template<typename T, typename E>
Reference<T> write_reference(
  WriteContext<E> *ctx,
  T const *pointer
) noexcept;
```

Write a single structure or primitive, returning a "reference".

Parameters:
- `ctx`: a pointer to the write context (see `write_start`).
- `pointer`: a pointer to the value. Its bytes will be written to output exactly as is.

Returns a "reference" to the written value. This reference can be used inside other structures to be written later.

### `write_sequence`
```cpp
template<typename T, typename E>
Sequence<T> write_sequence(
  WriteContext<E> *ctx,
  T const *pointer,
  uint32_t count
) noexcept;
```

Writes an array of structures or primitives, returning a "sequence".

Parameters:
- `ctx`: a pointer to the write context (see `write_start`).
- `pointer`: a pointer to the first value. Other values are presumed to be stored contiguously, and accessible via C-style array indexing of type `T`. The bytes will be written to output exactly as is.
- `count`: the total number of values to be written.

Returns a "sequence" of the written values. This sequence can be used inside other structures to be written later.

### `write_fixed_size_array`
```cpp
template<typename T, typename E, int N>
Sequence<T> write_fixed_size_array(
  WriteContext<E> *ctx,
  T const (&array)[N]
) noexcept;
```

Convenience function, only applicable to C-style arrays with length known at compile time.
Writes an array of structures or primitives, returning a "sequence".

Parameters:
- `ctx`: a pointer to the write context (see `write_start`).
- `array`: a C++ reference to a fixed-size array.

Returns a "sequence" of the written values. This sequence can be used inside other structures to be written later.

### `write_fixed_size_string`
```cpp
template<typename S, typename T, typename E, int N>
Sequence<S> write_fixed_size_string(
  WriteContext<E> *ctx,
  T const (&array)[N],
  bool termination_type = !0
);
```

Convenience function, similar to `write_fixed_size_array`, but tailored for string-like arrays. Should only be used with arrays of primitve types, typically `U8`, `U16` or `U32`. By default assumes no null-termination. However, if null-termination is specified, then the last element in the array is ignored.

- `ctx`: a pointer to the write context (see `write_start`).
- `array`: a C++ reference to a fixed-size array of primitives.
- `termination_type`: when 0, specifies the string to be null-terminated.

Returns a "sequence" of the written values. This sequence can be used inside other structures to be written later.

### `write_sequence_element`
```cpp
template<typename T, typename E>
void write_sequence_element(
  WriteContext<E> *ctx,
  T const *pointer,
  Sequence<T> *inout_sequence
);
```

Write a sequence element-by-element. When writing a sequence, other writes should not happen in-between (this will be detected and error flag set).

Parameters:
- `ctx`: a pointer to the write context (see `write_start`).
- `pointer`: a pointer to the value. Its bytes will be written to output exactly as is.
- `inout_sequence` *(in-out)*: to be modified during the write. For the first element, zero-initialize this first.

### `write_finish`
```cpp
template<typename T>
void write_finish(
  WriteContext<T> *ctx,
  T const *pointer
) noexcept;
```

Write the entry structure, which should be the last data written in the message. After calling this,
no other calls to `write_*` functions should be made for the current message. Check the `ctx->finished` flag to make sure everything is as expected (otherwise, `ctx->error_code` would be set).

Parameters:
- `ctx`: a pointer to the write context (see `write_start`).
- `pointer`: a pointer to the entry structure. Its bytes will be written to output exactly as is.

## C

Indended to be used with C99 and higher. Some of the API has to resort to MACROS (as customary, with all-caps naming), whenever that is the only way to reduce boilerplate for typical usage. These macros always call the underlying functions with matching names, and these functions can always be called directly.

---

- [`SVFRT_read_message`](#svfrt_read_message)
- [`SVFRT_SET_DEFAULT_READ_PARAMS`](#svfrt_set_default_read_params-macro)
- [`SVFRT_READ_REFERENCE`](#svfrt_read_reference-macro)
- [`SVFRT_READ_SEQUENCE_ELEMENT`](#svfrt_read_sequence_element-macro)
- [`SVFRT_READ_SEQUENCE_RAW`](#svfrt_read_sequence_raw-macro)
- [`SVFRT_read_reference`](#svfrt_read_reference)
- [`SVFRT_read_sequence_element`](#svfrt_read_sequence_element)
- [`SVFRT_read_sequence_raw`](#svfrt_read_sequence_raw)

---

- [`SVFRT_WRITE_START`](#svfrt_write_start-macro)
- [`SVFRT_WRITE_REFERENCE`](#svfrt_write_reference-macro)
- [`SVFRT_WRITE_FIXED_SIZE_ARRAY`](#svfrt_write_fixed_size_array-macro)
- [`SVFRT_WRITE_FIXED_SIZE_STRING`](#svfrt_write_fixed_size_string-macro)
- [`SVFRT_WRITE_SEQUENCE_ELEMENT`](#svfrt_write_sequence_element-macro)
- [`SVFRT_WRITE_FINISH`](#svfrt_write_finish-macro)
- [`SVFRT_write_start`](#svfrt_write_start)
- [`SVFRT_write_reference`](#svfrt_write_reference)
- [`SVFRT_write_sequence`](#svfrt_write_sequence)
- [`SVFRT_write_sequence_element`](#svfrt_write_sequence_element)
- [`SVFRT_write_finish`](#svfrt_write_finish)

---

### `SVFRT_read_message`
```c
void SVFRT_read_message(
  SVFRT_ReadMessageParams *params,
  SVFRT_ReadMessageResult *out_result,
  SVFRT_Bytes message,
  SVFRT_Bytes scratch
);
```

Read a message according to an expected schema. May fail, returning an error code in `out_result`.

Parameters:
- `params`: various input parameters grouped into a structure. Intended to almost always be initialized with `SVFRT_SET_DEFAULT_READ_PARAMS`, and possibly modified manually. Some the header file for full details.
- `params->required_level`: the required compatibility level between the expected schema, and the actual schema (one of: logical, binary, exact). `SVFRT_read_message` will fail, if the resulting compatibility is weaker than required.
- `params->allocator_fn`: an allocator function, necessary for logical compatibility, for which an allocation might be needed. `read_message` will call this function at most once.
- `params->allocator_ptr`: will be passed as the first argument to `allocator_fn`.
- `params->schema_lookup_fn`: when using incomplete messages with the schema omitted, this should be provided to look up a schema from its hash. The message will be interpreted according to the resulting schema. `read_message` will call this function at most once.
- `params->schema_lookup_ptr`: will be passed as the first argument to `schema_lookup_fn`.
- `out_result`: see below.
- `message`: the whole SVF message, as a contiguous buffer.
- `scratch`: a buffer with scratch memory. See `min_read_scratch_memory_size` in the generated header file of your schema. Typically should be allocated on the stack (as the size is known at compile time).

Returns (via out parameter `out_result`) a structure that should be used for subsequent reading operations.

### `SVFRT_SET_DEFAULT_READ_PARAMS` (macro)
```c
#define SVFRT_SET_DEFAULT_READ_PARAMS(out_params, schema_name, entry_name) /*...*/
```

A convenience macro to initialize the parameters for `SVFRT_read_message`.

Macro parameters:
- `out_params`: a pointer to a `SVFRT_ReadMessage_Params` structure, which be be initialized with "default" values.
- `schema_name`: the prefix of identifiers in the generated header, e.g. `SVF_SomeSchema`.
- `entry_name`: the identifier of the generated entry structure type, e.g. `SVF_SomeSchema_SomeEntry`.

### `SVFRT_READ_REFERENCE` (macro)
```c
#define SVFRT_READ_REFERENCE(type_name, ctx, reference) /*...*/
```

A convenience macro to access a reference of a specific type.

| **Warning:** since there are no generics in C, you must specify the type explicitly,
and if the specified type does not match the actual type, reading may return
either a valid pointer with junk content, or `NULL`. However, at no point any
memory will be accessed outside of the message data.

Macro parameters:
- `type_name`: the identifier of the type to read. It must be a primitive, or a struct type.
- `ctx`: a pointer to the read context (see `SVFRT_read_message`).
- `reference`: the reference to read.

Returns a valid pointer to type named `type_name`, if the reference is valid, or `NULL` otherwise.

### `SVFRT_READ_SEQUENCE_ELEMENT` (macro)
```c
#define SVFRT_READ_SEQUENCE_ELEMENT(type_name, ctx, sequence, element_index) /*...*/
```

A convenience macro to access a single element from a sequence.

| **Warning:** since there are no generics in C, you must specify the type explicitly,
and if the specified type does not match the actual type, reading may return
either a valid pointer with junk content, or `NULL`. However, at no point any
memory will be accessed outside of the message data.

Macro parameters:
- `type_name`: the identifier of the type to read. It must be a primitive, or a struct type.
- `ctx`: a pointer to the read context (see `SVFRT_read_message`).
- `sequence`: the sequence to read.
- `element_index`: the index of the element to read.

Returns a valid array element pointer, if the sequence is valid and contains the index, or `NULL` otherwise.

### `SVFRT_READ_SEQUENCE_RAW` (macro)
```c
#define SVFRT_READ_SEQUENCE_RAW(type_name, ctx, sequence) /*...*/
```

A convenience macro to access all elements in a sequence. This macro has limitations, as either:
- `type_name` must refer to primitive type.
- `type_name` must refer to a struct type, and "exact" or "logical" compatibility mode must have been specified when calling `SVFRT_read_message`.

The intended usage is mainly for string-like data, e.g. a sequence of `U8`, `U16`, or `U32`.

Macro parameters:
- `type_name`: the identifier of the type to read. It must be a primitive, or a struct type.
- `ctx`: a pointer to the read context (see `SVFRT_read_message`).
- `sequence`: the sequence to read.

Returns a valid pointer to the first C array element, if the sequence is valid, or `NULL` otherwise. The array is implicitly assumed to be of size `sequence.count`.

### `SVFRT_WRITE_START` (macro)
```c
#define SVFRT_WRITE_START(schema_name, entry_name, out_ctx, writer_fn, writer_ptr) /*...*/
```

A convenience macro to start writing a message. Will write the header and schema, and prepare everything for further operations.

Macro parameters:

- `schema_name`: the prefix of identifiers in the generated header, e.g. `SVF_SomeSchema`.
- `entry_name`: the identifier of the generated entry structure type, e.g. `SVF_SomeSchema_SomeEntry`.
- `out_ctx`: see below.
- `writer_fn`: a function which will actually write designated bytes to the output, be it a file, buffer, or otherwise. Should return the number of bytes actually written, or 0 on error. May be called an arbitrary number of times during the subsequent writing operations.
- `writer_ptr`: a pointer to be passed to `writer_fn`.

Returns the write context (via `out_ctx`), used for all subsequent operations. The `error_code` inside the context may be set on an error (see `SVFRT_code_write_*` in the headers), either immediately, or after subsequent writing operations.

### `SVFRT_WRITE_REFERENCE` (macro)
```c
#define SVFRT_WRITE_REFERENCE(ctx, data_ptr) /*...*/
```
Write a single structure or primitive, returning a "reference".

Parameters:
- `ctx`: a pointer to the write context (see `SVFRT_WRITE_START`).
- `data_ptr`: a pointer to the value. Its bytes will be written to output exactly as is.

Returns a "reference" to the written value. This reference can be used inside other structures to be written later.

### `SVFRT_WRITE_FIXED_SIZE_ARRAY` (macro)
```c
#define SVFRT_WRITE_FIXED_SIZE_ARRAY(ctx, array) /*...*/
```
Convenience macro, only applicable to C-style arrays with length known at compile time.
Writes an array of structures or primitives, returning a "sequence".

Macro parameters:
- `ctx`: a pointer to the write context (see `SVFRT_WRITE_START`).
- `array`: a fixed-size array.

Returns a "sequence" of the written values. This sequence can be used inside other structures to be written later.

### `SVFRT_WRITE_FIXED_SIZE_STRING` (macro)
```c
#define SVFRT_WRITE_FIXED_SIZE_STRING(ctx, array, termination_type) /*...*/
```

Convenience macro, similar to `SVFRT_WRITE_FIXED_SIZE_ARRAY`, but tailored for string-like arrays. Should only be used with arrays of primitve types, typically `U8`, `U16` or `U32`. If null-termination is specified, then the last element in the array is ignored.

- `ctx`: a pointer to the write context (see `SVFRT_WRITE_START`).
- `array`: a fixed-size array.
- `termination_type`: when 0, specifies the string to be null-terminated.

Returns a "sequence" of the written values. This sequence can be used inside other structures to be written later.

### `SVFRT_WRITE_SEQUENCE_ELEMENT` (macro)
```c
#define SVFRT_WRITE_SEQUENCE_ELEMENT(ctx, data_ptr, inout_sequence) /*...*/
```

Write a sequence element-by-element. When writing a sequence, other writes should not happen in-between (this will be detected and error flag set).

Parameters:
- `ctx`: a pointer to the write context (see `SVFRT_WRITE_START`).
- `data_ptr`: a pointer to the value. Its bytes will be written to output exactly as is.
- `inout_sequence` *(in-out)*: to be modified during the write. For the first element, zero-initialize this first.

### `SVFRT_WRITE_FINISH` (macro)
```c
#define SVFRT_WRITE_FINISH(ctx, data_ptr) /*...*/
```

Write the entry structure, which should be the last data written in the message. After calling this,
no other calls to `SVFRT_write_*` functions should be made for the current message. Check the `ctx->finished` flag to make sure everything is as expected (otherwise, `ctx->error_code` would be set).

| *Warning*: the type of this structure must correspond to the one specified in the message header, typically
via `SVFRT_WRITE_START`. A mistake here will likely lead to the whole message being interpreted by readers as erroneous, or containing junk.

Parameters:
- `ctx`: a pointer to the write context (see `SVFRT_WRITE_START`).
- `data_ptr`: a pointer to the entry structure. Its bytes will be written to output exactly as is.

### `SVFRT_read_reference`
```c
void const *SVFRT_read_reference(
  SVFRT_ReadContext *ctx,
  SVFRT_Reference reference,
  uint32_t type_size
);
```

Access a reference of a type with known size. Not intended to be used directly;
use the `SVFRT_READ_REFERENCE` macro instead.

Parameters:
- `ctx`: a pointer to the read context (see `SVFRT_read_message`).
- `reference`: the reference to read.
- `type_size`: size in bytes of the type.

Returns a valid `void` pointer, if the reference is valid, or `NULL` otherwise.

### `SVFRT_read_sequence_element`
```c
void const *SVFRT_read_sequence_element(
  SVFRT_ReadContext *ctx,
  SVFRT_Sequence sequence,
  uint32_t struct_index,
  uint32_t element_index
);
```

Access a single element from a sequence. Not intended to be used directly;
use the `SVFRT_READ_REFERENCE` macro instead.

Macro parameters:
- `ctx`: a pointer to the read context (see `SVFRT_read_message`).
- `sequence`: the sequence to read.
- `struct_index`: the struct index inside the schema. Normally is generated in the schema headers as e.g. `SVF_SomeSchema_SomeStruct_struct_index`.
- `element_index`: the index of the element to read.

Returns a valid pointer, if the sequence is valid and contains the index, or `NULL` otherwise.

### `SVFRT_read_sequence_raw`
```c
void const *SVFRT_read_sequence_raw(
  SVFRT_ReadContext *ctx,
  SVFRT_Sequence sequence,
  uint32_t type_stride
);
```
Access all elements in a sequence, when you know the stride. Not intended to be used directly;
use the `SVFRT_READ_SEQUENECE_RAW` macro instead.

Macro parameters:
- `ctx`: a pointer to the read context (see `SVFRT_read_message`).
- `sequence`: the sequence to read.
- `type_stride`: the stride in bytes between adjacent sequence elements.

Returns a valid pointer to the first C array element, if the sequence is valid, or `NULL` otherwise. The array is implicitly assumed to be of size `sequence.count`.

### `SVFRT_write_start`
```c
void SVFRT_write_start(
  SVFRT_WriteContext *result,
  SVFRT_WriterFn *writer_fn,
  void *writer_ptr,
  uint64_t schema_content_hash,
  SVFRT_Bytes schema_bytes,
  SVFRT_Bytes appendix_bytes,
  uint64_t entry_struct_id
);
```

Start writing a message. Will write the header and schema, and prepare everything for further operations.
Not intended to be used directly; use the `SVFRT_WRITE_START` macro instead.

Parameters:
- `result`: see below.
- `writer_fn`: a function which will actually write designated bytes to the output, be it a file, buffer, or otherwise. Should return the number of bytes actually written, or 0 on error. May be called an arbitrary number of times during the subsequent writing operations.
- `writer_ptr`: a pointer to be passed to `writer_fn`.
- `schema_content_hash`: a hash identifying the contents of the schema. See the generated schema header, e.g. `SVF_SomeSchema_schema_content_hash`.
- `schema_bytes`: the binary representation of the schema. See the generated schema header, e.g. `SVF_SomeSchema_schema_binary_array`.
- `appendix_bytes`: the binary representation of the schema appendix. Leave it empty.
- `entry_struct_id`: the entry ID of the struct intended to be the entry struct.

Returns (via `result`) the write context, used for all subsequent operations. The `error_code` inside the context may be set on an error (see `SVFRT_code_write_*` in the headers), either immediately, or after subsequent writing operations.

### `SVFRT_write_reference`
```c
SVFRT_Reference SVFRT_write_reference(
  SVFRT_WriteContext *ctx,
  void *pointer,
  uint32_t type_size
);
```

Write a single structure or primitive, returning a "reference".
Not intended to be used directly; use the `SVFRT_WRITE_REFERENCE` macro instead.

Parameters:
- `ctx`: a pointer to the write context (see `SVFRT_write_start`).
- `pointer`: a `void` pointer to the value. Its bytes will be written to output exactly as is.
- `type_size`: the size of the pointed-to value.

Returns a "reference" to the written value. This reference can be used inside other structures to be written later.

### `SVFRT_write_sequence`
```c
SVFRT_Sequence SVFRT_write_sequence(
  SVFRT_WriteContext *ctx,
  void *pointer,
  uint32_t type_size,
  uint32_t count
);
```

Writes an array of structures or primitives, returning a "sequence".
Not intended to be used directly; use the `SVFRT_WRITE_FIXED_SIZE_ARRAY` or `SVFRT_WRITE_FIXED_SIZE_STRING` macros instead.

Parameters:
- `ctx`: a pointer to the write context (see `SVFRT_write_start`).
- `pointer`: a pointer to the first value. Other values are presumed to be stored contiguously, with stride `type_size`. The bytes will be written to output exactly as is.
- `type_size`: the size of one array element, also used as the stride in the array.
- `count`: the total number of values to be written.

Returns a "sequence" of the written values. This sequence can be used inside other structures to be written later.

### `SVFRT_write_sequence_element`
```c
void SVFRT_write_sequence_element(
  SVFRT_WriteContext *ctx,
  void *pointer,
  uint32_t type_size,
  SVFRT_Sequence *inout_sequence
);
```

Write a sequence element-by-element. When writing a sequence, other writes should not happen in-between (this will be detected and error flag set).
Not intended to be used directly; use the `SVFRT_WRITE_SEQUENCE_ELEMENT` macro instead.

Parameters:
- `ctx`: a pointer to the write context (see `SVFRT_write_start`).
- `pointer`: a pointer to the value. Its bytes will be written to output exactly as is.
- `type_size`: the size of the pointed-to value.
- `inout_sequence` *(in-out)*: to be modified during the write. For the first element, zero-initialize this first.

### `SVFRT_write_finish`
```c
void SVFRT_write_finish(
  SVFRT_WriteContext *ctx,
  void *pointer,
  uint32_t type_size
);
```

Write the entry structure, which should be the last data written in the message. After calling this,
no other calls to `write_*` functions should be made for the current message. Check the `ctx->finished` flag to make sure everything is as expected (otherwise, `ctx->error_code` would be set).

Not intended to be used directly; use the `SVFRT_WRITE_FINISH` macro instead.

| *Warning*: the type of this structure must correspond to the one specified in the message header, typically
via `SVFRT_WRITE_START`. A mistake here will likely lead to the whole message being interpreted by readers as erroneous, or containing junk.

Parameters:
- `ctx`: a pointer to the write context (see `SVFRT_write_start`).
- `pointer`: a pointer to the entry structure. Its bytes will be written to output exactly as is.
- `type_size`: the size of the pointed-to value.
