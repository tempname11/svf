// AUTOGENERATED by svfc.
#ifndef SVF_Meta_H
#define SVF_Meta_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>

extern "C" {
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifndef SVF_COMMON_C_TYPES_INCLUDED
#define SVF_COMMON_C_TYPES_INCLUDED
#pragma pack(push, 1)

typedef struct SVFRT_Reference {
  uint32_t data_offset_complement;
} SVFRT_Reference;

typedef struct SVFRT_Sequence {
  uint32_t data_offset_complement;
  uint32_t count;
} SVFRT_Sequence;

#pragma pack(pop)
#endif // SVF_COMMON_C_TYPES_INCLUDED

#pragma pack(push, 1)

#define SVF_Meta_min_read_scratch_memory_size 219
#define SVF_Meta_compatibility_work_base 67
#define SVF_Meta_schema_binary_size 1294
#define SVF_Meta_schema_id 0x6DADEAAEE49D6D18ull
#define SVF_Meta_schema_content_hash 0x6C539EFA421B06BBull
extern uint8_t const SVF_Meta_schema_binary_array[];
extern uint32_t const SVF_Meta_schema_struct_strides[];
#define SVF_Meta_schema_struct_count 12

// Forward declarations.
typedef struct SVF_Meta_SchemaDefinition SVF_Meta_SchemaDefinition;
typedef struct SVF_Meta_ChoiceDefinition SVF_Meta_ChoiceDefinition;
typedef struct SVF_Meta_StructDefinition SVF_Meta_StructDefinition;
typedef struct SVF_Meta_ConcreteType_DefinedStruct SVF_Meta_ConcreteType_DefinedStruct;
typedef struct SVF_Meta_ConcreteType_DefinedChoice SVF_Meta_ConcreteType_DefinedChoice;
typedef struct SVF_Meta_Appendix SVF_Meta_Appendix;
typedef struct SVF_Meta_NameMapping SVF_Meta_NameMapping;
typedef struct SVF_Meta_Type_Concrete SVF_Meta_Type_Concrete;
typedef struct SVF_Meta_Type_Reference SVF_Meta_Type_Reference;
typedef struct SVF_Meta_Type_Sequence SVF_Meta_Type_Sequence;
typedef struct SVF_Meta_OptionDefinition SVF_Meta_OptionDefinition;
typedef struct SVF_Meta_FieldDefinition SVF_Meta_FieldDefinition;
typedef uint8_t SVF_Meta_ConcreteType_tag;
typedef union SVF_Meta_ConcreteType_payload SVF_Meta_ConcreteType_payload;
typedef uint8_t SVF_Meta_Type_tag;
typedef union SVF_Meta_Type_payload SVF_Meta_Type_payload;

// Indexes of structs.
#define SVF_Meta_SchemaDefinition_struct_index 0
#define SVF_Meta_ChoiceDefinition_struct_index 1
#define SVF_Meta_StructDefinition_struct_index 2
#define SVF_Meta_ConcreteType_DefinedStruct_struct_index 3
#define SVF_Meta_ConcreteType_DefinedChoice_struct_index 4
#define SVF_Meta_Appendix_struct_index 5
#define SVF_Meta_NameMapping_struct_index 6
#define SVF_Meta_Type_Concrete_struct_index 7
#define SVF_Meta_Type_Reference_struct_index 8
#define SVF_Meta_Type_Sequence_struct_index 9
#define SVF_Meta_OptionDefinition_struct_index 10
#define SVF_Meta_FieldDefinition_struct_index 11

// Hashes of top level definition names.
#define SVF_Meta_SchemaDefinition_type_id 0x85B94A79B2A1A5EFull
#define SVF_Meta_ChoiceDefinition_type_id 0x2240FF3EC854982Full
#define SVF_Meta_StructDefinition_type_id 0x713C0B32A28A6581ull
#define SVF_Meta_ConcreteType_DefinedStruct_type_id 0xE1EBFBC1CB324605ull
#define SVF_Meta_ConcreteType_DefinedChoice_type_id 0x20ADB239462DD81Full
#define SVF_Meta_Appendix_type_id 0xAEB58B70AFC09880ull
#define SVF_Meta_NameMapping_type_id 0xDF6C2AFBE80F7A98ull
#define SVF_Meta_Type_Concrete_type_id 0xAD0D45DB75A2937Dull
#define SVF_Meta_Type_Reference_type_id 0x4CE48FE156562743ull
#define SVF_Meta_Type_Sequence_type_id 0x9E1FB822B59C8E77ull
#define SVF_Meta_OptionDefinition_type_id 0x1F70FAEE117DDC5Dull
#define SVF_Meta_FieldDefinition_type_id 0xDF03D0229D043C3Aull
#define SVF_Meta_ConcreteType_type_id 0x698D4BD276D7869Eull
#define SVF_Meta_Type_type_id 0xD2223AFB7D6B100Dull

// Full declarations.
struct SVF_Meta_SchemaDefinition {
  uint64_t schemaId;
  SVFRT_Sequence /*SVF_Meta_StructDefinition*/ structs;
  SVFRT_Sequence /*SVF_Meta_ChoiceDefinition*/ choices;
};

struct SVF_Meta_ChoiceDefinition {
  uint64_t typeId;
  uint32_t payloadSize;
  SVFRT_Sequence /*SVF_Meta_OptionDefinition*/ options;
};

struct SVF_Meta_StructDefinition {
  uint64_t typeId;
  uint32_t size;
  SVFRT_Sequence /*SVF_Meta_FieldDefinition*/ fields;
};

struct SVF_Meta_ConcreteType_DefinedStruct {
  uint32_t index;
};

struct SVF_Meta_ConcreteType_DefinedChoice {
  uint32_t index;
};

struct SVF_Meta_Appendix {
  SVFRT_Sequence /*SVF_Meta_NameMapping*/ names;
};

struct SVF_Meta_NameMapping {
  uint64_t id;
  SVFRT_Sequence /*uint8_t*/ name;
};

#define SVF_Meta_ConcreteType_tag_u8 0
#define SVF_Meta_ConcreteType_tag_u16 1
#define SVF_Meta_ConcreteType_tag_u32 2
#define SVF_Meta_ConcreteType_tag_u64 3
#define SVF_Meta_ConcreteType_tag_i8 4
#define SVF_Meta_ConcreteType_tag_i16 5
#define SVF_Meta_ConcreteType_tag_i32 6
#define SVF_Meta_ConcreteType_tag_i64 7
#define SVF_Meta_ConcreteType_tag_f32 8
#define SVF_Meta_ConcreteType_tag_f64 9
#define SVF_Meta_ConcreteType_tag_zeroSized 10
#define SVF_Meta_ConcreteType_tag_definedStruct 11
#define SVF_Meta_ConcreteType_tag_definedChoice 12

union SVF_Meta_ConcreteType_payload {
  SVF_Meta_ConcreteType_DefinedStruct definedStruct;
  SVF_Meta_ConcreteType_DefinedChoice definedChoice;
};

struct SVF_Meta_Type_Concrete {
  SVF_Meta_ConcreteType_tag type_tag;
  SVF_Meta_ConcreteType_payload type_payload;
};

struct SVF_Meta_Type_Reference {
  SVF_Meta_ConcreteType_tag type_tag;
  SVF_Meta_ConcreteType_payload type_payload;
};

struct SVF_Meta_Type_Sequence {
  SVF_Meta_ConcreteType_tag elementType_tag;
  SVF_Meta_ConcreteType_payload elementType_payload;
};

#define SVF_Meta_Type_tag_concrete 0
#define SVF_Meta_Type_tag_reference 1
#define SVF_Meta_Type_tag_sequence 2

union SVF_Meta_Type_payload {
  SVF_Meta_Type_Concrete concrete;
  SVF_Meta_Type_Reference reference;
  SVF_Meta_Type_Sequence sequence;
};

struct SVF_Meta_OptionDefinition {
  uint64_t optionId;
  uint8_t tag;
  SVF_Meta_Type_tag type_tag;
  SVF_Meta_Type_payload type_payload;
};

struct SVF_Meta_FieldDefinition {
  uint64_t fieldId;
  uint32_t offset;
  SVF_Meta_Type_tag type_tag;
  SVF_Meta_Type_payload type_payload;
};

#pragma pack(pop)

// Binary schema.
#if defined(SVF_INCLUDE_BINARY_SCHEMA) || defined(SVF_IMPLEMENTATION)
#ifndef SVF_Meta_BINARY_INCLUDED_H
uint32_t const SVF_Meta_schema_struct_strides[] = {
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
  15,
  18
};

uint8_t const SVF_Meta_schema_binary_array[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
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
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xEF, 0xA5, 0xA1, 0xB2, 0x79, 0x4A, 0xB9, 0x85,
  0x18, 0x00, 0x00, 0x00, 0x97, 0xFD, 0xFF, 0xFF,
  0x03, 0x00, 0x00, 0x00, 0x2F, 0x98, 0x54, 0xC8,
  0x3E, 0xFF, 0x40, 0x22, 0x14, 0x00, 0x00, 0x00,
  0x61, 0xFD, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
  0x81, 0x65, 0x8A, 0xA2, 0x32, 0x0B, 0x3C, 0x71,
  0x14, 0x00, 0x00, 0x00, 0x2B, 0xFD, 0xFF, 0xFF,
  0x03, 0x00, 0x00, 0x00, 0x05, 0x46, 0x32, 0xCB,
  0xC1, 0xFB, 0xEB, 0xE1, 0x04, 0x00, 0x00, 0x00,
  0xF5, 0xFC, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0x1F, 0xD8, 0x2D, 0x46, 0x39, 0xB2, 0xAD, 0x20,
  0x04, 0x00, 0x00, 0x00, 0xE3, 0xFC, 0xFF, 0xFF,
  0x01, 0x00, 0x00, 0x00, 0x80, 0x98, 0xC0, 0xAF,
  0x70, 0x8B, 0xB5, 0xAE, 0x08, 0x00, 0x00, 0x00,
  0xD1, 0xFC, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0x98, 0x7A, 0x0F, 0xE8, 0xFB, 0x2A, 0x6C, 0xDF,
  0x10, 0x00, 0x00, 0x00, 0xBF, 0xFC, 0xFF, 0xFF,
  0x02, 0x00, 0x00, 0x00, 0x7D, 0x93, 0xA2, 0x75,
  0xDB, 0x45, 0x0D, 0xAD, 0x05, 0x00, 0x00, 0x00,
  0xD8, 0xFB, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0x43, 0x27, 0x56, 0x56, 0xE1, 0x8F, 0xE4, 0x4C,
  0x05, 0x00, 0x00, 0x00, 0xC6, 0xFB, 0xFF, 0xFF,
  0x01, 0x00, 0x00, 0x00, 0x77, 0x8E, 0x9C, 0xB5,
  0x22, 0xB8, 0x1F, 0x9E, 0x05, 0x00, 0x00, 0x00,
  0xB4, 0xFB, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00,
  0x5D, 0xDC, 0x7D, 0x11, 0xEE, 0xFA, 0x70, 0x1F,
  0x0F, 0x00, 0x00, 0x00, 0x75, 0xFB, 0xFF, 0xFF,
  0x03, 0x00, 0x00, 0x00, 0x3A, 0x3C, 0x04, 0x9D,
  0x22, 0xD0, 0x03, 0xDF, 0x12, 0x00, 0x00, 0x00,
  0x3F, 0xFB, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
  0x9E, 0x86, 0xD7, 0x76, 0xD2, 0x4B, 0x8D, 0x69,
  0x04, 0x00, 0x00, 0x00, 0x9B, 0xFC, 0xFF, 0xFF,
  0x0D, 0x00, 0x00, 0x00, 0x0D, 0x10, 0x6B, 0x7D,
  0xFB, 0x3A, 0x22, 0xD2, 0x05, 0x00, 0x00, 0x00,
  0xA2, 0xFB, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
  0xAB, 0x87, 0x7B, 0x2F, 0x57, 0xC0, 0x4B, 0x65,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
  0x00, 0x00, 0x09, 0xA2, 0x22, 0x4C, 0x0B, 0xEE,
  0xFF, 0x1B, 0x08, 0x00, 0x00, 0x00, 0x02, 0x0B,
  0x02, 0x00, 0x00, 0x00, 0xFF, 0x0D, 0x9C, 0x63,
  0xA9, 0x58, 0x57, 0x72, 0x10, 0x00, 0x00, 0x00,
  0x02, 0x0B, 0x01, 0x00, 0x00, 0x00, 0x18, 0x0E,
  0x97, 0x4C, 0x3F, 0x0E, 0x77, 0x69, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
  0xDC, 0xD1, 0x84, 0x28, 0x1E, 0x41, 0x5A, 0x44,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
  0x00, 0x00, 0x45, 0x87, 0xAD, 0x44, 0x66, 0xF4,
  0x45, 0xCA, 0x0C, 0x00, 0x00, 0x00, 0x02, 0x0B,
  0x0A, 0x00, 0x00, 0x00, 0x18, 0x0E, 0x97, 0x4C,
  0x3F, 0x0E, 0x77, 0x69, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x3C, 0xAE,
  0x18, 0xE6, 0x18, 0x96, 0xEA, 0x4D, 0x08, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
  0xDC, 0x7E, 0x84, 0x24, 0x6D, 0x59, 0x0E, 0x49,
  0x0C, 0x00, 0x00, 0x00, 0x02, 0x0B, 0x0B, 0x00,
  0x00, 0x00, 0x8B, 0x46, 0x81, 0x90, 0x8F, 0x8E,
  0xCF, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x8B, 0x46, 0x81, 0x90,
  0x8F, 0x8E, 0xCF, 0x83, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x4F, 0x81,
  0x68, 0xF2, 0xFF, 0x28, 0xB7, 0xAF, 0x00, 0x00,
  0x00, 0x00, 0x02, 0x0B, 0x06, 0x00, 0x00, 0x00,
  0xC0, 0x3A, 0x5C, 0xB5, 0x07, 0x2E, 0xB7, 0x08,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
  0x00, 0x00, 0x86, 0x1B, 0x63, 0x8E, 0xBA, 0xAD,
  0xBC, 0xC4, 0x08, 0x00, 0x00, 0x00, 0x02, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xD8, 0x53, 0x67, 0xB5,
  0x07, 0x82, 0xC4, 0x08, 0x00, 0x00, 0x0A, 0x00,
  0x00, 0x00, 0x00, 0xBF, 0xF3, 0x7E, 0x3E, 0x19,
  0xD3, 0x24, 0x4D, 0x01, 0x00, 0x0A, 0x00, 0x00,
  0x00, 0x00, 0xD1, 0x26, 0x85, 0x3E, 0x19, 0xDF,
  0x2B, 0x4D, 0x02, 0x00, 0x0A, 0x00, 0x00, 0x00,
  0x00, 0xF2, 0x66, 0x8D, 0x3E, 0x19, 0xD3, 0x35,
  0x4D, 0x03, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00,
  0x94, 0xFD, 0x5B, 0xB5, 0x07, 0x0A, 0xB7, 0x08,
  0x04, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0xFB,
  0x86, 0x3B, 0x2B, 0x19, 0xBF, 0xEB, 0x2A, 0x05,
  0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x45, 0x91,
  0x41, 0x2B, 0x19, 0xB3, 0xF2, 0x2A, 0x06, 0x00,
  0x0A, 0x00, 0x00, 0x00, 0x00, 0x46, 0x17, 0x33,
  0x2B, 0x19, 0xAF, 0xE1, 0x2A, 0x07, 0x00, 0x0A,
  0x00, 0x00, 0x00, 0x00, 0x44, 0x35, 0x70, 0xFF,
  0x18, 0x50, 0x63, 0xDD, 0x08, 0x00, 0x0A, 0x00,
  0x00, 0x00, 0x00, 0xAB, 0xA1, 0x7E, 0xFF, 0x18,
  0x4C, 0x74, 0xDD, 0x09, 0x00, 0x0A, 0x00, 0x00,
  0x00, 0x00, 0x72, 0x52, 0xF6, 0xE8, 0x9D, 0xB6,
  0x75, 0x1E, 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x00,
  0x00, 0xC7, 0x36, 0xCE, 0x96, 0x23, 0xC0, 0x3C,
  0xC3, 0x0B, 0x00, 0x0B, 0x03, 0x00, 0x00, 0x00,
  0x71, 0x09, 0x00, 0x60, 0x83, 0xDB, 0x79, 0x4C,
  0x0C, 0x00, 0x0B, 0x04, 0x00, 0x00, 0x00, 0x2D,
  0x9C, 0xFA, 0x7B, 0xEF, 0x39, 0x94, 0xA7, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00,
  0x00, 0x2D, 0x9C, 0xFA, 0x7B, 0xEF, 0x39, 0x94,
  0xA7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00,
  0x00, 0x00, 0x00, 0x6F, 0x6D, 0xB4, 0x9B, 0x75,
  0xFD, 0xD3, 0xA9, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0C, 0x00, 0x00, 0x00, 0x00, 0x1E, 0xD9, 0xC5,
  0x8B, 0xC7, 0x71, 0x89, 0xCA, 0x00, 0x00, 0x0B,
  0x07, 0x00, 0x00, 0x00, 0x7A, 0xBA, 0xA7, 0x62,
  0x32, 0x10, 0x7B, 0x9A, 0x01, 0x00, 0x0B, 0x08,
  0x00, 0x00, 0x00, 0xA8, 0x28, 0xF5, 0x81, 0xA4,
  0xAC, 0x38, 0xAA, 0x02, 0x00, 0x0B, 0x09, 0x00,
  0x00, 0x00, 0x79, 0xBD, 0xBF, 0xB2, 0xC6, 0x6E,
  0x43, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
  0x00, 0x00, 0x00, 0x00, 0xF3, 0xA4, 0x48, 0x44,
  0x19, 0xAB, 0xD7, 0x56, 0x08, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x9C,
  0xFA, 0x7B, 0xEF, 0x39, 0x94, 0xA7, 0x09, 0x00,
  0x00, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00, 0x00,
  0x2A, 0xA8, 0xB5, 0x0C, 0x75, 0x90, 0x5F, 0xA7,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
  0x00, 0x00, 0xCA, 0x35, 0x94, 0x12, 0xF8, 0xB0,
  0x68, 0x02, 0x08, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x2D, 0x9C, 0xFA, 0x7B,
  0xEF, 0x39, 0x94, 0xA7, 0x0C, 0x00, 0x00, 0x00,
  0x00, 0x0C, 0x01, 0x00, 0x00, 0x00, 0x18, 0x6D,
  0x9D, 0xE4, 0xAE, 0xEA, 0xAD, 0x6D, 0xAF, 0xFE,
  0xFF, 0xFF, 0x0C, 0x00, 0x00, 0x00, 0xBF, 0xFD,
  0xFF, 0xFF, 0x02, 0x00, 0x00, 0x00
};
#endif // SVF_Meta_BINARY_INCLUDED_H
#endif // defined(SVF_INCLUDE_BINARY_SCHEMA) || defined(SVF_IMPLEMENTATION)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SVF_Meta_H
