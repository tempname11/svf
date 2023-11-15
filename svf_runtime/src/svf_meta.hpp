// AUTOGENERATED by svfc.
#pragma once
#include <cstdint>
#include <cstddef>

namespace svf {

#ifndef SVF_COMMON_CPP_TYPES_INCLUDED
#define SVF_COMMON_CPP_TYPES_INCLUDED
#pragma pack(push, 1)
namespace runtime {

template<typename T>
struct Reference {
  uint32_t data_offset_complement;
};

template<typename T>
struct Sequence {
  uint32_t data_offset_complement;
  uint32_t count;
};

template<typename T> struct GetSchemaFromType;

} // namespace runtime
#pragma pack(pop)
#endif // SVF_COMMON_CPP_TYPES_INCLUDED

namespace Meta {
#pragma pack(push, 1)

extern uint32_t const struct_strides[];

namespace binary {
  size_t const size = 1019;
  extern uint8_t const array[];
} // namespace binary

// Forward declarations.
struct SchemaDefinition;
struct ChoiceDefinition;
struct StructDefinition;
struct ConcreteType_DefinedStruct;
struct ConcreteType_DefinedChoice;
struct Appendix;
struct NameMapping;
struct Type_Concrete;
struct Type_Reference;
struct Type_Sequence;
struct OptionDefinition;
struct FieldDefinition;
enum class ConcreteType_tag: uint8_t;
union ConcreteType_payload;
enum class Type_tag: uint8_t;
union Type_payload;

// Indexes of structs.
uint32_t const SchemaDefinition_struct_index = 0;
uint32_t const ChoiceDefinition_struct_index = 1;
uint32_t const StructDefinition_struct_index = 2;
uint32_t const ConcreteType_DefinedStruct_struct_index = 3;
uint32_t const ConcreteType_DefinedChoice_struct_index = 4;
uint32_t const Appendix_struct_index = 5;
uint32_t const NameMapping_struct_index = 6;
uint32_t const Type_Concrete_struct_index = 7;
uint32_t const Type_Reference_struct_index = 8;
uint32_t const Type_Sequence_struct_index = 9;
uint32_t const OptionDefinition_struct_index = 10;
uint32_t const FieldDefinition_struct_index = 11;

// Hashes of top level definition names.
uint64_t const SchemaDefinition_type_id = 0x85B94A79B2A1A5EFull;
uint64_t const ChoiceDefinition_type_id = 0x2240FF3EC854982Full;
uint64_t const StructDefinition_type_id = 0x713C0B32A28A6581ull;
uint64_t const ConcreteType_DefinedStruct_type_id = 0xE1EBFBC1CB324605ull;
uint64_t const ConcreteType_DefinedChoice_type_id = 0x20ADB239462DD81Full;
uint64_t const Appendix_type_id = 0xAEB58B70AFC09880ull;
uint64_t const NameMapping_type_id = 0xDF6C2AFBE80F7A98ull;
uint64_t const Type_Concrete_type_id = 0xAD0D45DB75A2937Dull;
uint64_t const Type_Reference_type_id = 0x4CE48FE156562743ull;
uint64_t const Type_Sequence_type_id = 0x9E1FB822B59C8E77ull;
uint64_t const OptionDefinition_type_id = 0x1F70FAEE117DDC5Dull;
uint64_t const FieldDefinition_type_id = 0xDF03D0229D043C3Aull;
uint64_t const ConcreteType_type_id = 0x698D4BD276D7869Eull;
uint64_t const Type_type_id = 0xD2223AFB7D6B100Dull;

// Full declarations.
struct SchemaDefinition {
  uint64_t schemaId;
  runtime::Sequence<StructDefinition> structs;
  runtime::Sequence<ChoiceDefinition> choices;
};

struct ChoiceDefinition {
  uint64_t typeId;
  uint32_t payloadSize;
  runtime::Sequence<OptionDefinition> options;
};

struct StructDefinition {
  uint64_t typeId;
  uint32_t size;
  runtime::Sequence<FieldDefinition> fields;
};

struct ConcreteType_DefinedStruct {
  uint32_t index;
};

struct ConcreteType_DefinedChoice {
  uint32_t index;
};

struct Appendix {
  runtime::Sequence<NameMapping> names;
};

struct NameMapping {
  uint64_t id;
  runtime::Sequence<uint8_t> name;
};

enum class ConcreteType_tag: uint8_t {
  nothing = 0,
  u8 = 1,
  u16 = 2,
  u32 = 3,
  u64 = 4,
  i8 = 5,
  i16 = 6,
  i32 = 7,
  i64 = 8,
  f32 = 9,
  f64 = 10,
  definedStruct = 11,
  definedChoice = 12,
};

union ConcreteType_payload {
  ConcreteType_DefinedStruct definedStruct;
  ConcreteType_DefinedChoice definedChoice;
};

struct Type_Concrete {
  ConcreteType_tag type_tag;
  ConcreteType_payload type_payload;
};

struct Type_Reference {
  ConcreteType_tag type_tag;
  ConcreteType_payload type_payload;
};

struct Type_Sequence {
  ConcreteType_tag elementType_tag;
  ConcreteType_payload elementType_payload;
};

enum class Type_tag: uint8_t {
  nothing = 0,
  concrete = 1,
  reference = 2,
  sequence = 3,
};

union Type_payload {
  Type_Concrete concrete;
  Type_Reference reference;
  Type_Sequence sequence;
};

struct OptionDefinition {
  uint64_t optionId;
  uint8_t tag;
  Type_tag type_tag;
  Type_payload type_payload;
  uint8_t removed;
};

struct FieldDefinition {
  uint64_t fieldId;
  uint32_t offset;
  Type_tag type_tag;
  Type_payload type_payload;
  uint8_t removed;
};

#pragma pack(pop)

// C++ trickery: _SchemaDescription.
struct _SchemaDescription {
  template<typename T>
  struct PerType;

  static constexpr uint32_t *schema_struct_strides = (uint32_t *) struct_strides;
  static constexpr uint8_t *schema_binary_array = (uint8_t *) binary::array;
  static constexpr size_t schema_binary_size = binary::size;
  static constexpr uint32_t schema_struct_count = 12;
  static constexpr uint32_t min_read_scratch_memory_size = 450;
  static constexpr uint32_t compatibility_work_base = 456;
  static constexpr uint64_t schema_id = 0x6DADEAAEE49D6D18ull;
  static constexpr uint64_t content_hash = 0x5AB083CE4C8A2D3Bull;
};

// C++ trickery: _SchemaDescription::PerType.
template<>
struct _SchemaDescription::PerType<SchemaDefinition> {
  static constexpr uint64_t type_id = SchemaDefinition_type_id;
  static constexpr uint32_t index = SchemaDefinition_struct_index;
};

template<>
struct _SchemaDescription::PerType<ChoiceDefinition> {
  static constexpr uint64_t type_id = ChoiceDefinition_type_id;
  static constexpr uint32_t index = ChoiceDefinition_struct_index;
};

template<>
struct _SchemaDescription::PerType<StructDefinition> {
  static constexpr uint64_t type_id = StructDefinition_type_id;
  static constexpr uint32_t index = StructDefinition_struct_index;
};

template<>
struct _SchemaDescription::PerType<ConcreteType_DefinedStruct> {
  static constexpr uint64_t type_id = ConcreteType_DefinedStruct_type_id;
  static constexpr uint32_t index = ConcreteType_DefinedStruct_struct_index;
};

template<>
struct _SchemaDescription::PerType<ConcreteType_DefinedChoice> {
  static constexpr uint64_t type_id = ConcreteType_DefinedChoice_type_id;
  static constexpr uint32_t index = ConcreteType_DefinedChoice_struct_index;
};

template<>
struct _SchemaDescription::PerType<Appendix> {
  static constexpr uint64_t type_id = Appendix_type_id;
  static constexpr uint32_t index = Appendix_struct_index;
};

template<>
struct _SchemaDescription::PerType<NameMapping> {
  static constexpr uint64_t type_id = NameMapping_type_id;
  static constexpr uint32_t index = NameMapping_struct_index;
};

template<>
struct _SchemaDescription::PerType<Type_Concrete> {
  static constexpr uint64_t type_id = Type_Concrete_type_id;
  static constexpr uint32_t index = Type_Concrete_struct_index;
};

template<>
struct _SchemaDescription::PerType<Type_Reference> {
  static constexpr uint64_t type_id = Type_Reference_type_id;
  static constexpr uint32_t index = Type_Reference_struct_index;
};

template<>
struct _SchemaDescription::PerType<Type_Sequence> {
  static constexpr uint64_t type_id = Type_Sequence_type_id;
  static constexpr uint32_t index = Type_Sequence_struct_index;
};

template<>
struct _SchemaDescription::PerType<OptionDefinition> {
  static constexpr uint64_t type_id = OptionDefinition_type_id;
  static constexpr uint32_t index = OptionDefinition_struct_index;
};

template<>
struct _SchemaDescription::PerType<FieldDefinition> {
  static constexpr uint64_t type_id = FieldDefinition_type_id;
  static constexpr uint32_t index = FieldDefinition_struct_index;
};

} // namespace Meta

namespace runtime {

// C++ trickery: GetSchemaFromType.
template<>
struct GetSchemaFromType<Meta::SchemaDefinition> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::ChoiceDefinition> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::StructDefinition> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::ConcreteType_DefinedStruct> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::ConcreteType_DefinedChoice> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::Appendix> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::NameMapping> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::Type_Concrete> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::Type_Reference> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::Type_Sequence> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::OptionDefinition> {
  using SchemaDescription = Meta::_SchemaDescription;
};

template<>
struct GetSchemaFromType<Meta::FieldDefinition> {
  using SchemaDescription = Meta::_SchemaDescription;
};

} // namespace runtime

// Binary schema.
#if defined(SVF_INCLUDE_BINARY_SCHEMA) || defined(SVF_IMPLEMENTATION)
#ifndef SVF_Meta_BINARY_INCLUDED_HPP
namespace Meta {

uint32_t const struct_strides[] = {
  24,
  20,
  20,
  4,
  4,
  8,
  16,
  5,
  5,
  5,
  16,
  19
};

namespace binary {

uint8_t const array[] = {
  0xEF, 0xA5, 0xA1, 0xB2, 0x79, 0x4A, 0xB9, 0x85,
  0x18, 0x00, 0x00, 0x00, 0xE7, 0xFE, 0xFF, 0xFF,
  0x03, 0x00, 0x00, 0x00, 0x2F, 0x98, 0x54, 0xC8,
  0x3E, 0xFF, 0x40, 0x22, 0x14, 0x00, 0x00, 0x00,
  0xAE, 0xFE, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
  0x81, 0x65, 0x8A, 0xA2, 0x32, 0x0B, 0x3C, 0x71,
  0x14, 0x00, 0x00, 0x00, 0x75, 0xFE, 0xFF, 0xFF,
  0x03, 0x00, 0x00, 0x00, 0x05, 0x46, 0x32, 0xCB,
  0xC1, 0xFB, 0xEB, 0xE1, 0x04, 0x00, 0x00, 0x00,
  0x3C, 0xFE, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0x1F, 0xD8, 0x2D, 0x46, 0x39, 0xB2, 0xAD, 0x20,
  0x04, 0x00, 0x00, 0x00, 0x29, 0xFE, 0xFF, 0xFF,
  0x01, 0x00, 0x00, 0x00, 0x80, 0x98, 0xC0, 0xAF,
  0x70, 0x8B, 0xB5, 0xAE, 0x08, 0x00, 0x00, 0x00,
  0x16, 0xFE, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0x98, 0x7A, 0x0F, 0xE8, 0xFB, 0x2A, 0x6C, 0xDF,
  0x10, 0x00, 0x00, 0x00, 0x03, 0xFE, 0xFF, 0xFF,
  0x02, 0x00, 0x00, 0x00, 0x7D, 0x93, 0xA2, 0x75,
  0xDB, 0x45, 0x0D, 0xAD, 0x05, 0x00, 0x00, 0x00,
  0x1D, 0xFD, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0x43, 0x27, 0x56, 0x56, 0xE1, 0x8F, 0xE4, 0x4C,
  0x05, 0x00, 0x00, 0x00, 0x0A, 0xFD, 0xFF, 0xFF,
  0x01, 0x00, 0x00, 0x00, 0x77, 0x8E, 0x9C, 0xB5,
  0x22, 0xB8, 0x1F, 0x9E, 0x05, 0x00, 0x00, 0x00,
  0xF7, 0xFC, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0x5D, 0xDC, 0x7D, 0x11, 0xEE, 0xFA, 0x70, 0x1F,
  0x10, 0x00, 0x00, 0x00, 0xB4, 0xFC, 0xFF, 0xFF,
  0x04, 0x00, 0x00, 0x00, 0x3A, 0x3C, 0x04, 0x9D,
  0x22, 0xD0, 0x03, 0xDF, 0x13, 0x00, 0x00, 0x00,
  0x68, 0xFC, 0xFF, 0xFF, 0x04, 0x00, 0x00, 0x00,
  0x9E, 0x86, 0xD7, 0x76, 0xD2, 0x4B, 0x8D, 0x69,
  0x04, 0x00, 0x00, 0x00, 0xDD, 0xFD, 0xFF, 0xFF,
  0x0C, 0x00, 0x00, 0x00, 0x0D, 0x10, 0x6B, 0x7D,
  0xFB, 0x3A, 0x22, 0xD2, 0x05, 0x00, 0x00, 0x00,
  0xE4, 0xFC, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
  0xAB, 0x87, 0x7B, 0x2F, 0x57, 0xC0, 0x4B, 0x65,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x09, 0xA2, 0x22, 0x4C, 0x0B,
  0xEE, 0xFF, 0x1B, 0x08, 0x00, 0x00, 0x00, 0x03,
  0x0B, 0x02, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x0D,
  0x9C, 0x63, 0xA9, 0x58, 0x57, 0x72, 0x10, 0x00,
  0x00, 0x00, 0x03, 0x0B, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x18, 0x0E, 0x97, 0x4C, 0x3F, 0x0E, 0x77,
  0x69, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xDC, 0xD1, 0x84, 0x28,
  0x1E, 0x41, 0x5A, 0x44, 0x08, 0x00, 0x00, 0x00,
  0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45,
  0x87, 0xAD, 0x44, 0x66, 0xF4, 0x45, 0x4A, 0x0C,
  0x00, 0x00, 0x00, 0x03, 0x0B, 0x0A, 0x00, 0x00,
  0x00, 0x00, 0x18, 0x0E, 0x97, 0x4C, 0x3F, 0x0E,
  0x77, 0x69, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0xAE, 0x18,
  0xE6, 0x18, 0x96, 0xEA, 0x4D, 0x08, 0x00, 0x00,
  0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xDC, 0x7E, 0x84, 0x24, 0x6D, 0x59, 0x0E, 0x49,
  0x0C, 0x00, 0x00, 0x00, 0x03, 0x0B, 0x0B, 0x00,
  0x00, 0x00, 0x00, 0x8B, 0x46, 0x81, 0x90, 0x8F,
  0x8E, 0xCF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x46,
  0x81, 0x90, 0x8F, 0x8E, 0xCF, 0x03, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x4F, 0x81, 0x68, 0xF2, 0xFF, 0x28, 0xB7,
  0x2F, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0B, 0x06,
  0x00, 0x00, 0x00, 0x00, 0xC0, 0x3A, 0x5C, 0xB5,
  0x07, 0x2E, 0xB7, 0x08, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86,
  0x1B, 0x63, 0x8E, 0xBA, 0xAD, 0xBC, 0x44, 0x08,
  0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xD8, 0x53, 0x67, 0xB5, 0x07, 0x82,
  0xC4, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xBF, 0xF3, 0x7E, 0x3E, 0x19, 0xD3,
  0x24, 0x4D, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xD1, 0x26, 0x85, 0x3E, 0x19, 0xDF,
  0x2B, 0x4D, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xF2, 0x66, 0x8D, 0x3E, 0x19, 0xD3,
  0x35, 0x4D, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x94, 0xFD, 0x5B, 0xB5, 0x07, 0x0A,
  0xB7, 0x08, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xFB, 0x86, 0x3B, 0x2B, 0x19, 0xBF,
  0xEB, 0x2A, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x45, 0x91, 0x41, 0x2B, 0x19, 0xB3,
  0xF2, 0x2A, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x46, 0x17, 0x33, 0x2B, 0x19, 0xAF,
  0xE1, 0x2A, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x44, 0x35, 0x70, 0xFF, 0x18, 0x50,
  0x63, 0x5D, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xAB, 0xA1, 0x7E, 0xFF, 0x18, 0x4C,
  0x74, 0x5D, 0x0A, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xC7, 0x36, 0xCE, 0x96, 0x23, 0xC0,
  0x3C, 0x43, 0x0B, 0x01, 0x0B, 0x03, 0x00, 0x00,
  0x00, 0x00, 0x71, 0x09, 0x00, 0x60, 0x83, 0xDB,
  0x79, 0x4C, 0x0C, 0x01, 0x0B, 0x04, 0x00, 0x00,
  0x00, 0x00, 0x2D, 0x9C, 0xFA, 0x7B, 0xEF, 0x39,
  0x94, 0x27, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0C,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x9C, 0xFA,
  0x7B, 0xEF, 0x39, 0x94, 0x27, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x6F, 0x6D, 0xB4, 0x9B, 0x75, 0xFD, 0xD3, 0x29,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x0C, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x1E, 0xD9, 0xC5, 0x8B, 0xC7,
  0x71, 0x89, 0x4A, 0x01, 0x01, 0x0B, 0x07, 0x00,
  0x00, 0x00, 0x00, 0x7A, 0xBA, 0xA7, 0x62, 0x32,
  0x10, 0x7B, 0x1A, 0x02, 0x01, 0x0B, 0x08, 0x00,
  0x00, 0x00, 0x00, 0xA8, 0x28, 0xF5, 0x81, 0xA4,
  0xAC, 0x38, 0x2A, 0x03, 0x01, 0x0B, 0x09, 0x00,
  0x00, 0x00, 0x00, 0x79, 0xBD, 0xBF, 0xB2, 0xC6,
  0x6E, 0x43, 0x62, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF3, 0xA4,
  0x48, 0x44, 0x19, 0xAB, 0xD7, 0x56, 0x08, 0x00,
  0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x2D, 0x9C, 0xFA, 0x7B, 0xEF, 0x39, 0x94,
  0x27, 0x09, 0x00, 0x00, 0x00, 0x01, 0x0C, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x7B, 0x69, 0xBC, 0x4F,
  0xBD, 0x4D, 0x15, 0x10, 0x0F, 0x00, 0x00, 0x00,
  0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A,
  0xA8, 0xB5, 0x0C, 0x75, 0x90, 0x5F, 0x27, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xCA, 0x35, 0x94, 0x12, 0xF8, 0xB0,
  0x68, 0x02, 0x08, 0x00, 0x00, 0x00, 0x01, 0x03,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x9C, 0xFA,
  0x7B, 0xEF, 0x39, 0x94, 0x27, 0x0C, 0x00, 0x00,
  0x00, 0x01, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x7B, 0x69, 0xBC, 0x4F, 0xBD, 0x4D, 0x15, 0x10,
  0x12, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x18, 0x6D, 0x9D, 0xE4, 0xAE,
  0xEA, 0xAD, 0x6D, 0xFF, 0xFF, 0xFF, 0xFF, 0x0C,
  0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0x02,
  0x00, 0x00, 0x00
};

} // namespace binary
} // namespace Meta
#endif // SVF_Meta_BINARY_INCLUDED_HPP
#endif // defined(SVF_INCLUDE_BINARY_SCHEMA) || defined(SVF_IMPLEMENTATION)

} // namespace svf
