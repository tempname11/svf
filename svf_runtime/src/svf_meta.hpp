// AUTOGENERATED by svfc.
#pragma once
#include <cstdint>
#include <cstddef>

namespace svf {

#ifndef SVF_COMMON_CPP_TYPES_INCLUDED
#define SVF_COMMON_CPP_TYPES_INCLUDED

using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

using I8 = int8_t;
using I16 = int16_t;
using I32 = int32_t;
using I64 = int64_t;

using F32 = float;
using F64 = double;

#pragma pack(push, 1)

template<typename T>
struct Reference {
  U32 data_offset_complement;
};

template<typename T>
struct Sequence {
  U32 data_offset_complement;
  U32 count;
};

#pragma pack(pop)
#endif // SVF_COMMON_CPP_TYPES_INCLUDED

#ifndef SVF_COMMON_CPP_TRICKERY_INCLUDED
#define SVF_COMMON_CPP_TRICKERY_INCLUDED

template<typename T>
struct GetSchemaFromType;

#endif // SVF_COMMON_CPP_TRICKERY_INCLUDED

namespace META {
#pragma pack(push, 1)

namespace binary {
  size_t const size = 2133;
  extern U8 const array[];
} // namespace binary

// Forward declarations.
struct SchemaDefinition;
struct ChoiceDefinition;
struct StructDefinition;
struct ConcreteType_dot_defined_struct;
struct ConcreteType_dot_defined_choice;
struct Type_dot_concrete;
struct Type_dot_reference;
struct Type_dot_sequence;
struct OptionDefinition;
struct FieldDefinition;
enum class ConcreteType_enum: U8;
union ConcreteType_union;
enum class Type_enum: U8;
union Type_union;

// Indexes of structs.
U32 const SchemaDefinition_struct_index = 0;
U32 const ChoiceDefinition_struct_index = 1;
U32 const StructDefinition_struct_index = 2;
U32 const ConcreteType_dot_defined_struct_struct_index = 3;
U32 const ConcreteType_dot_defined_choice_struct_index = 4;
U32 const Type_dot_concrete_struct_index = 5;
U32 const Type_dot_reference_struct_index = 6;
U32 const Type_dot_sequence_struct_index = 7;
U32 const OptionDefinition_struct_index = 8;
U32 const FieldDefinition_struct_index = 9;

// Hashes of top level definition names.
U64 const SchemaDefinition_name_hash = 0x85B94A79B2A1A5EFull;
U64 const ChoiceDefinition_name_hash = 0x2240FF3EC854982Full;
U64 const StructDefinition_name_hash = 0x713C0B32A28A6581ull;
U64 const ConcreteType_dot_defined_struct_name_hash = 0x7B77971DFF8A9EE4ull;
U64 const ConcreteType_dot_defined_choice_name_hash = 0x0AE2DFC383A7B2E2ull;
U64 const Type_dot_concrete_name_hash = 0xD52542288532EF0Dull;
U64 const Type_dot_reference_name_hash = 0xE6F96540BF15F333ull;
U64 const Type_dot_sequence_name_hash = 0xFEBB3FE06B40EBE7ull;
U64 const OptionDefinition_name_hash = 0x1F70FAEE117DDC5Dull;
U64 const FieldDefinition_name_hash = 0xDF03D0229D043C3Aull;
U64 const ConcreteType_name_hash = 0x698D4BD276D7869Eull;
U64 const Type_name_hash = 0xD2223AFB7D6B100Dull;

// Full declarations.
struct SchemaDefinition {
  U64 name_hash;
  Sequence<U8> name;
  Sequence<StructDefinition> structs;
  Sequence<ChoiceDefinition> choices;
};

struct ChoiceDefinition {
  U64 name_hash;
  Sequence<U8> name;
  U32 payload_size;
  Sequence<OptionDefinition> options;
};

struct StructDefinition {
  U64 name_hash;
  Sequence<U8> name;
  U32 size;
  Sequence<FieldDefinition> fields;
};

struct ConcreteType_dot_defined_struct {
  U32 index;
};

struct ConcreteType_dot_defined_choice {
  U32 index;
};

enum class ConcreteType_enum: U8 {
  u8 = 0,
  u16 = 1,
  u32 = 2,
  u64 = 3,
  i8 = 4,
  i16 = 5,
  i32 = 6,
  i64 = 7,
  f32 = 8,
  f64 = 9,
  zero_sized = 10,
  defined_struct = 11,
  defined_choice = 12,
};

union ConcreteType_union {
  ConcreteType_dot_defined_struct defined_struct;
  ConcreteType_dot_defined_choice defined_choice;
};

struct Type_dot_concrete {
  ConcreteType_enum type_enum;
  ConcreteType_union type_union;
};

struct Type_dot_reference {
  ConcreteType_enum type_enum;
  ConcreteType_union type_union;
};

struct Type_dot_sequence {
  ConcreteType_enum element_type_enum;
  ConcreteType_union element_type_union;
};

enum class Type_enum: U8 {
  concrete = 0,
  reference = 1,
  sequence = 2,
};

union Type_union {
  Type_dot_concrete concrete;
  Type_dot_reference reference;
  Type_dot_sequence sequence;
};

struct OptionDefinition {
  U64 name_hash;
  Sequence<U8> name;
  U8 tag;
  Type_enum type_enum;
  Type_union type_union;
};

struct FieldDefinition {
  U64 name_hash;
  Sequence<U8> name;
  U32 offset;
  Type_enum type_enum;
  Type_union type_union;
};

#pragma pack(pop)

// C++ trickery: SchemaDescription.
struct SchemaDescription {
  template<typename T>
  struct PerType;

  static constexpr U8 *schema_binary_array = (U8 *) binary::array;
  static constexpr size_t schema_binary_size = binary::size;
  static constexpr U32 min_read_scratch_memory_size = 187;
  static constexpr U32 compatibility_work_base = 95;
  static constexpr U64 name_hash = 0x80BE2AAF7FD058B8ull;
  static constexpr U64 content_hash = 0xB8A328E5D3533E6Eull;
};

// C++ trickery: SchemaDescription::PerType.
template<>
struct SchemaDescription::PerType<SchemaDefinition> {
  static constexpr U64 name_hash = SchemaDefinition_name_hash;
  static constexpr U32 index = SchemaDefinition_struct_index;
};

template<>
struct SchemaDescription::PerType<ChoiceDefinition> {
  static constexpr U64 name_hash = ChoiceDefinition_name_hash;
  static constexpr U32 index = ChoiceDefinition_struct_index;
};

template<>
struct SchemaDescription::PerType<StructDefinition> {
  static constexpr U64 name_hash = StructDefinition_name_hash;
  static constexpr U32 index = StructDefinition_struct_index;
};

template<>
struct SchemaDescription::PerType<ConcreteType_dot_defined_struct> {
  static constexpr U64 name_hash = ConcreteType_dot_defined_struct_name_hash;
  static constexpr U32 index = ConcreteType_dot_defined_struct_struct_index;
};

template<>
struct SchemaDescription::PerType<ConcreteType_dot_defined_choice> {
  static constexpr U64 name_hash = ConcreteType_dot_defined_choice_name_hash;
  static constexpr U32 index = ConcreteType_dot_defined_choice_struct_index;
};

template<>
struct SchemaDescription::PerType<Type_dot_concrete> {
  static constexpr U64 name_hash = Type_dot_concrete_name_hash;
  static constexpr U32 index = Type_dot_concrete_struct_index;
};

template<>
struct SchemaDescription::PerType<Type_dot_reference> {
  static constexpr U64 name_hash = Type_dot_reference_name_hash;
  static constexpr U32 index = Type_dot_reference_struct_index;
};

template<>
struct SchemaDescription::PerType<Type_dot_sequence> {
  static constexpr U64 name_hash = Type_dot_sequence_name_hash;
  static constexpr U32 index = Type_dot_sequence_struct_index;
};

template<>
struct SchemaDescription::PerType<OptionDefinition> {
  static constexpr U64 name_hash = OptionDefinition_name_hash;
  static constexpr U32 index = OptionDefinition_struct_index;
};

template<>
struct SchemaDescription::PerType<FieldDefinition> {
  static constexpr U64 name_hash = FieldDefinition_name_hash;
  static constexpr U32 index = FieldDefinition_struct_index;
};

} // namespace META

// C++ trickery: GetSchemaFromType.
template<>
struct GetSchemaFromType<META::SchemaDefinition> {
  using SchemaDescription = META::SchemaDescription;
};

template<>
struct GetSchemaFromType<META::ChoiceDefinition> {
  using SchemaDescription = META::SchemaDescription;
};

template<>
struct GetSchemaFromType<META::StructDefinition> {
  using SchemaDescription = META::SchemaDescription;
};

template<>
struct GetSchemaFromType<META::ConcreteType_dot_defined_struct> {
  using SchemaDescription = META::SchemaDescription;
};

template<>
struct GetSchemaFromType<META::ConcreteType_dot_defined_choice> {
  using SchemaDescription = META::SchemaDescription;
};

template<>
struct GetSchemaFromType<META::Type_dot_concrete> {
  using SchemaDescription = META::SchemaDescription;
};

template<>
struct GetSchemaFromType<META::Type_dot_reference> {
  using SchemaDescription = META::SchemaDescription;
};

template<>
struct GetSchemaFromType<META::Type_dot_sequence> {
  using SchemaDescription = META::SchemaDescription;
};

template<>
struct GetSchemaFromType<META::OptionDefinition> {
  using SchemaDescription = META::SchemaDescription;
};

template<>
struct GetSchemaFromType<META::FieldDefinition> {
  using SchemaDescription = META::SchemaDescription;
};

// Binary schema.
#if defined(SVF_INCLUDE_BINARY_SCHEMA) || defined(SVF_IMPLEMENTATION)
#ifndef SVF_META_BINARY_INCLUDED_HPP
namespace META {
namespace binary {

U8 const array[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xEF, 0xA5, 0xA1, 0xB2, 0x79, 0x4A, 0xB9, 0x85,
  0x0C, 0xFD, 0xFF, 0xFF, 0x10, 0x00, 0x00, 0x00,
  0x20, 0x00, 0x00, 0x00, 0x8F, 0xFD, 0xFF, 0xFF,
  0x04, 0x00, 0x00, 0x00, 0x2F, 0x98, 0x54, 0xC8,
  0x3E, 0xFF, 0x40, 0x22, 0x74, 0xFC, 0xFF, 0xFF,
  0x10, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00,
  0xFC, 0xFC, 0xFF, 0xFF, 0x04, 0x00, 0x00, 0x00,
  0x81, 0x65, 0x8A, 0xA2, 0x32, 0x0B, 0x3C, 0x71,
  0xE5, 0xFB, 0xFF, 0xFF, 0x10, 0x00, 0x00, 0x00,
  0x1C, 0x00, 0x00, 0x00, 0x64, 0xFC, 0xFF, 0xFF,
  0x04, 0x00, 0x00, 0x00, 0xE4, 0x9E, 0x8A, 0xFF,
  0x1D, 0x97, 0x77, 0x7B, 0xB6, 0xFB, 0xFF, 0xFF,
  0x1F, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0xD5, 0xFB, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0xE2, 0xB2, 0xA7, 0x83, 0xC3, 0xDF, 0xE2, 0x0A,
  0x78, 0xFB, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x97, 0xFB, 0xFF, 0xFF,
  0x01, 0x00, 0x00, 0x00, 0x0D, 0xEF, 0x32, 0x85,
  0x28, 0x42, 0x25, 0xD5, 0xC2, 0xF9, 0xFF, 0xFF,
  0x11, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0xE0, 0xF9, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0x33, 0xF3, 0x15, 0xBF, 0x40, 0x65, 0xF9, 0xE6,
  0x93, 0xF9, 0xFF, 0xFF, 0x12, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0xB1, 0xF9, 0xFF, 0xFF,
  0x01, 0x00, 0x00, 0x00, 0xE7, 0xEB, 0x40, 0x6B,
  0xE0, 0x3F, 0xBB, 0xFE, 0x5B, 0xF9, 0xFF, 0xFF,
  0x11, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x81, 0xF9, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0x5D, 0xDC, 0x7D, 0x11, 0xEE, 0xFA, 0x70, 0x1F,
  0x6C, 0xF8, 0xFF, 0xFF, 0x10, 0x00, 0x00, 0x00,
  0x17, 0x00, 0x00, 0x00, 0xE8, 0xF8, 0xFF, 0xFF,
  0x04, 0x00, 0x00, 0x00, 0x3A, 0x3C, 0x04, 0x9D,
  0x22, 0xD0, 0x03, 0xDF, 0xDD, 0xF7, 0xFF, 0xFF,
  0x0F, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00,
  0x5C, 0xF8, 0xFF, 0xFF, 0x04, 0x00, 0x00, 0x00,
  0x9E, 0x86, 0xD7, 0x76, 0xD2, 0x4B, 0x8D, 0x69,
  0xEC, 0xF9, 0xFF, 0xFF, 0x0C, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x59, 0xFB, 0xFF, 0xFF,
  0x0D, 0x00, 0x00, 0x00, 0x0D, 0x10, 0x6B, 0x7D,
  0xFB, 0x3A, 0x22, 0xD2, 0xEC, 0xF8, 0xFF, 0xFF,
  0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  0x4A, 0xF9, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
  0x7B, 0x14, 0x2A, 0x8E, 0xAC, 0xBF, 0x15, 0x77,
  0x27, 0xFD, 0xFF, 0xFF, 0x09, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
  0x00, 0x00, 0x86, 0x1B, 0x63, 0x8E, 0xBA, 0xAD,
  0xBC, 0xC4, 0x1E, 0xFD, 0xFF, 0xFF, 0x04, 0x00,
  0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x09, 0xA2, 0x22, 0x4C,
  0x0B, 0xEE, 0xFF, 0x1B, 0x1A, 0xFD, 0xFF, 0xFF,
  0x07, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x02, 0x0B, 0x02, 0x00, 0x00, 0x00, 0xFF, 0x0D,
  0x9C, 0x63, 0xA9, 0x58, 0x57, 0x72, 0x13, 0xFD,
  0xFF, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x18, 0x00,
  0x00, 0x00, 0x02, 0x0B, 0x01, 0x00, 0x00, 0x00,
  0x6E, 0x61, 0x6D, 0x65, 0x5F, 0x68, 0x61, 0x73,
  0x68, 0x6E, 0x61, 0x6D, 0x65, 0x73, 0x74, 0x72,
  0x75, 0x63, 0x74, 0x73, 0x63, 0x68, 0x6F, 0x69,
  0x63, 0x65, 0x73, 0x53, 0x63, 0x68, 0x65, 0x6D,
  0x61, 0x44, 0x65, 0x66, 0x69, 0x6E, 0x69, 0x74,
  0x69, 0x6F, 0x6E, 0x7B, 0x14, 0x2A, 0x8E, 0xAC,
  0xBF, 0x15, 0x77, 0x94, 0xFC, 0xFF, 0xFF, 0x09,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x86, 0x1B, 0x63,
  0x8E, 0xBA, 0xAD, 0xBC, 0xC4, 0x8B, 0xFC, 0xFF,
  0xFF, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCF,
  0x39, 0xE1, 0x80, 0x42, 0xB1, 0xA9, 0x8B, 0x87,
  0xFC, 0xFF, 0xFF, 0x0C, 0x00, 0x00, 0x00, 0x10,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x00, 0x45, 0x87, 0xAD, 0x44, 0x66, 0xF4, 0x45,
  0xCA, 0x7B, 0xFC, 0xFF, 0xFF, 0x07, 0x00, 0x00,
  0x00, 0x14, 0x00, 0x00, 0x00, 0x02, 0x0B, 0x08,
  0x00, 0x00, 0x00, 0x6E, 0x61, 0x6D, 0x65, 0x5F,
  0x68, 0x61, 0x73, 0x68, 0x6E, 0x61, 0x6D, 0x65,
  0x70, 0x61, 0x79, 0x6C, 0x6F, 0x61, 0x64, 0x5F,
  0x73, 0x69, 0x7A, 0x65, 0x6F, 0x70, 0x74, 0x69,
  0x6F, 0x6E, 0x73, 0x43, 0x68, 0x6F, 0x69, 0x63,
  0x65, 0x44, 0x65, 0x66, 0x69, 0x6E, 0x69, 0x74,
  0x69, 0x6F, 0x6E, 0x7B, 0x14, 0x2A, 0x8E, 0xAC,
  0xBF, 0x15, 0x77, 0xFC, 0xFB, 0xFF, 0xFF, 0x09,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x86, 0x1B, 0x63,
  0x8E, 0xBA, 0xAD, 0xBC, 0xC4, 0xF3, 0xFB, 0xFF,
  0xFF, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C,
  0xAE, 0x18, 0xE6, 0x18, 0x96, 0xEA, 0x4D, 0xEF,
  0xFB, 0xFF, 0xFF, 0x04, 0x00, 0x00, 0x00, 0x10,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x00, 0xDC, 0x7E, 0x84, 0x24, 0x6D, 0x59, 0x0E,
  0x49, 0xEB, 0xFB, 0xFF, 0xFF, 0x06, 0x00, 0x00,
  0x00, 0x14, 0x00, 0x00, 0x00, 0x02, 0x0B, 0x09,
  0x00, 0x00, 0x00, 0x6E, 0x61, 0x6D, 0x65, 0x5F,
  0x68, 0x61, 0x73, 0x68, 0x6E, 0x61, 0x6D, 0x65,
  0x73, 0x69, 0x7A, 0x65, 0x66, 0x69, 0x65, 0x6C,
  0x64, 0x73, 0x53, 0x74, 0x72, 0x75, 0x63, 0x74,
  0x44, 0x65, 0x66, 0x69, 0x6E, 0x69, 0x74, 0x69,
  0x6F, 0x6E, 0x8B, 0x46, 0x81, 0x90, 0x8F, 0x8E,
  0xCF, 0x83, 0xBB, 0xFB, 0xFF, 0xFF, 0x05, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x69, 0x6E, 0x64, 0x65,
  0x78, 0x43, 0x6F, 0x6E, 0x63, 0x72, 0x65, 0x74,
  0x65, 0x54, 0x79, 0x70, 0x65, 0x5F, 0x64, 0x6F,
  0x74, 0x5F, 0x64, 0x65, 0x66, 0x69, 0x6E, 0x65,
  0x64, 0x5F, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74,
  0x8B, 0x46, 0x81, 0x90, 0x8F, 0x8E, 0xCF, 0x83,
  0x7D, 0xFB, 0xFF, 0xFF, 0x05, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
  0x00, 0x00, 0x69, 0x6E, 0x64, 0x65, 0x78, 0x43,
  0x6F, 0x6E, 0x63, 0x72, 0x65, 0x74, 0x65, 0x54,
  0x79, 0x70, 0x65, 0x5F, 0x64, 0x6F, 0x74, 0x5F,
  0x64, 0x65, 0x66, 0x69, 0x6E, 0x65, 0x64, 0x5F,
  0x63, 0x68, 0x6F, 0x69, 0x63, 0x65, 0xD8, 0x53,
  0x67, 0xB5, 0x07, 0x82, 0xC4, 0x08, 0x2E, 0xFA,
  0xFF, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0A, 0x00, 0x00, 0x00, 0x00, 0xBF, 0xF3, 0x7E,
  0x3E, 0x19, 0xD3, 0x24, 0x4D, 0x2C, 0xFA, 0xFF,
  0xFF, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0A,
  0x00, 0x00, 0x00, 0x00, 0xD1, 0x26, 0x85, 0x3E,
  0x19, 0xDF, 0x2B, 0x4D, 0x29, 0xFA, 0xFF, 0xFF,
  0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x0A, 0x00,
  0x00, 0x00, 0x00, 0xF2, 0x66, 0x8D, 0x3E, 0x19,
  0xD3, 0x35, 0x4D, 0x26, 0xFA, 0xFF, 0xFF, 0x03,
  0x00, 0x00, 0x00, 0x03, 0x00, 0x0A, 0x00, 0x00,
  0x00, 0x00, 0x94, 0xFD, 0x5B, 0xB5, 0x07, 0x0A,
  0xB7, 0x08, 0x23, 0xFA, 0xFF, 0xFF, 0x02, 0x00,
  0x00, 0x00, 0x04, 0x00, 0x0A, 0x00, 0x00, 0x00,
  0x00, 0xFB, 0x86, 0x3B, 0x2B, 0x19, 0xBF, 0xEB,
  0x2A, 0x21, 0xFA, 0xFF, 0xFF, 0x03, 0x00, 0x00,
  0x00, 0x05, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00,
  0x45, 0x91, 0x41, 0x2B, 0x19, 0xB3, 0xF2, 0x2A,
  0x1E, 0xFA, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
  0x06, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x46,
  0x17, 0x33, 0x2B, 0x19, 0xAF, 0xE1, 0x2A, 0x1B,
  0xFA, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x07,
  0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x44, 0x35,
  0x70, 0xFF, 0x18, 0x50, 0x63, 0xDD, 0x18, 0xFA,
  0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00,
  0x0A, 0x00, 0x00, 0x00, 0x00, 0xAB, 0xA1, 0x7E,
  0xFF, 0x18, 0x4C, 0x74, 0xDD, 0x15, 0xFA, 0xFF,
  0xFF, 0x03, 0x00, 0x00, 0x00, 0x09, 0x00, 0x0A,
  0x00, 0x00, 0x00, 0x00, 0x6B, 0x35, 0x6A, 0x71,
  0xD6, 0x75, 0x16, 0x5E, 0x12, 0xFA, 0xFF, 0xFF,
  0x0A, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x0A, 0x00,
  0x00, 0x00, 0x00, 0x06, 0x9E, 0x0C, 0x45, 0xCF,
  0x3C, 0x24, 0x6D, 0x08, 0xFA, 0xFF, 0xFF, 0x0E,
  0x00, 0x00, 0x00, 0x0B, 0x00, 0x0B, 0x03, 0x00,
  0x00, 0x00, 0x84, 0x16, 0x3F, 0x2A, 0xD2, 0xCA,
  0x9B, 0xDD, 0xFA, 0xF9, 0xFF, 0xFF, 0x0E, 0x00,
  0x00, 0x00, 0x0C, 0x00, 0x0B, 0x04, 0x00, 0x00,
  0x00, 0x75, 0x38, 0x75, 0x31, 0x36, 0x75, 0x33,
  0x32, 0x75, 0x36, 0x34, 0x69, 0x38, 0x69, 0x31,
  0x36, 0x69, 0x33, 0x32, 0x69, 0x36, 0x34, 0x66,
  0x33, 0x32, 0x66, 0x36, 0x34, 0x7A, 0x65, 0x72,
  0x6F, 0x5F, 0x73, 0x69, 0x7A, 0x65, 0x64, 0x64,
  0x65, 0x66, 0x69, 0x6E, 0x65, 0x64, 0x5F, 0x73,
  0x74, 0x72, 0x75, 0x63, 0x74, 0x64, 0x65, 0x66,
  0x69, 0x6E, 0x65, 0x64, 0x5F, 0x63, 0x68, 0x6F,
  0x69, 0x63, 0x65, 0x43, 0x6F, 0x6E, 0x63, 0x72,
  0x65, 0x74, 0x65, 0x54, 0x79, 0x70, 0x65, 0x2D,
  0x9C, 0xFA, 0x7B, 0xEF, 0x39, 0x94, 0xA7, 0xC6,
  0xF9, 0xFF, 0xFF, 0x04, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
  0x00, 0x74, 0x79, 0x70, 0x65, 0x54, 0x79, 0x70,
  0x65, 0x5F, 0x64, 0x6F, 0x74, 0x5F, 0x63, 0x6F,
  0x6E, 0x63, 0x72, 0x65, 0x74, 0x65, 0x2D, 0x9C,
  0xFA, 0x7B, 0xEF, 0x39, 0x94, 0xA7, 0x97, 0xF9,
  0xFF, 0xFF, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00,
  0x74, 0x79, 0x70, 0x65, 0x54, 0x79, 0x70, 0x65,
  0x5F, 0x64, 0x6F, 0x74, 0x5F, 0x72, 0x65, 0x66,
  0x65, 0x72, 0x65, 0x6E, 0x63, 0x65, 0x90, 0xBD,
  0x24, 0xCD, 0x19, 0x9A, 0x43, 0x1F, 0x67, 0xF9,
  0xFF, 0xFF, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00,
  0x65, 0x6C, 0x65, 0x6D, 0x65, 0x6E, 0x74, 0x5F,
  0x74, 0x79, 0x70, 0x65, 0x54, 0x79, 0x70, 0x65,
  0x5F, 0x64, 0x6F, 0x74, 0x5F, 0x73, 0x65, 0x71,
  0x75, 0x65, 0x6E, 0x63, 0x65, 0x1E, 0xD9, 0xC5,
  0x8B, 0xC7, 0x71, 0x89, 0xCA, 0x05, 0xF9, 0xFF,
  0xFF, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B,
  0x05, 0x00, 0x00, 0x00, 0x7A, 0xBA, 0xA7, 0x62,
  0x32, 0x10, 0x7B, 0x9A, 0xFD, 0xF8, 0xFF, 0xFF,
  0x09, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0B, 0x06,
  0x00, 0x00, 0x00, 0xA8, 0x28, 0xF5, 0x81, 0xA4,
  0xAC, 0x38, 0xAA, 0xF4, 0xF8, 0xFF, 0xFF, 0x08,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x0B, 0x07, 0x00,
  0x00, 0x00, 0x63, 0x6F, 0x6E, 0x63, 0x72, 0x65,
  0x74, 0x65, 0x72, 0x65, 0x66, 0x65, 0x72, 0x65,
  0x6E, 0x63, 0x65, 0x73, 0x65, 0x71, 0x75, 0x65,
  0x6E, 0x63, 0x65, 0x54, 0x79, 0x70, 0x65, 0x7B,
  0x14, 0x2A, 0x8E, 0xAC, 0xBF, 0x15, 0x77, 0x80,
  0xF8, 0xFF, 0xFF, 0x09, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x00, 0x86, 0x1B, 0x63, 0x8E, 0xBA, 0xAD, 0xBC,
  0xC4, 0x77, 0xF8, 0xFF, 0xFF, 0x04, 0x00, 0x00,
  0x00, 0x08, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xF3, 0xA4, 0x48, 0x44, 0x19,
  0xAB, 0xD7, 0x56, 0x73, 0xF8, 0xFF, 0xFF, 0x03,
  0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x9C, 0xFA,
  0x7B, 0xEF, 0x39, 0x94, 0xA7, 0x70, 0xF8, 0xFF,
  0xFF, 0x04, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00,
  0x00, 0x00, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x6E,
  0x61, 0x6D, 0x65, 0x5F, 0x68, 0x61, 0x73, 0x68,
  0x6E, 0x61, 0x6D, 0x65, 0x74, 0x61, 0x67, 0x74,
  0x79, 0x70, 0x65, 0x4F, 0x70, 0x74, 0x69, 0x6F,
  0x6E, 0x44, 0x65, 0x66, 0x69, 0x6E, 0x69, 0x74,
  0x69, 0x6F, 0x6E, 0x7B, 0x14, 0x2A, 0x8E, 0xAC,
  0xBF, 0x15, 0x77, 0xF4, 0xF7, 0xFF, 0xFF, 0x09,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x86, 0x1B, 0x63,
  0x8E, 0xBA, 0xAD, 0xBC, 0xC4, 0xEB, 0xF7, 0xFF,
  0xFF, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCA,
  0x35, 0x94, 0x12, 0xF8, 0xB0, 0x68, 0x02, 0xE7,
  0xF7, 0xFF, 0xFF, 0x06, 0x00, 0x00, 0x00, 0x10,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x00, 0x2D, 0x9C, 0xFA, 0x7B, 0xEF, 0x39, 0x94,
  0xA7, 0xE1, 0xF7, 0xFF, 0xFF, 0x04, 0x00, 0x00,
  0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x01,
  0x00, 0x00, 0x00, 0x6E, 0x61, 0x6D, 0x65, 0x5F,
  0x68, 0x61, 0x73, 0x68, 0x6E, 0x61, 0x6D, 0x65,
  0x6F, 0x66, 0x66, 0x73, 0x65, 0x74, 0x74, 0x79,
  0x70, 0x65, 0x46, 0x69, 0x65, 0x6C, 0x64, 0x44,
  0x65, 0x66, 0x69, 0x6E, 0x69, 0x74, 0x69, 0x6F,
  0x6E, 0x4D, 0x45, 0x54, 0x41, 0xB8, 0x58, 0xD0,
  0x7F, 0xAF, 0x2A, 0xBE, 0x80, 0xCE, 0xF7, 0xFF,
  0xFF, 0x04, 0x00, 0x00, 0x00, 0xDF, 0xFE, 0xFF,
  0xFF, 0x0A, 0x00, 0x00, 0x00, 0xC7, 0xFD, 0xFF,
  0xFF, 0x02, 0x00, 0x00, 0x00
};

} // namespace binary
} // namespace META
#endif // SVF_META_BINARY_INCLUDED_HPP
#endif // defined(SVF_INCLUDE_BINARY_SCHEMA) || defined(SVF_IMPLEMENTATION)

} // namespace svf
