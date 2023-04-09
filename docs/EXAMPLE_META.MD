# Metaschema

The "metaschema" is the schema for the schema description format itself:

```c
#name schema_format_0

EntryPoint: struct {
  structs: StructDefinition[];
  choices: ChoiceDefinition[];
};

StructDefinition: struct {
  name_hash: U64;
  fields: FieldDefinition[];
};

FieldDefinition: struct {
  name_hash: U64;
  type: Type;
};

ChoiceDefinition: struct {
  name_hash: U64;
  options: OptionDefinition[];
};

OptionDefinition: struct {
  name_hash: U64;
  index: U8;
  type: Type;
};

ConcreteType: choice {
  u8;
  u16;
  u32;
  u64;
  i8;
  i16;
  i32;
  i64;
  f32;
  f64;
  defined: struct {
    top_level_definition_index: U32;
  };
};

Type: choice {
  concrete: struct {
    type: ConcreteType;
  };
  pointer: struct {
    type: ConcreteType;
  };
  fixed_size_array: struct {
    count: U8;
    element_type: ConcreteType;
  };
  flexible_array: struct {
    element_type: ConcreteType;
  };
};
```

will be transformed to C++ code:
```c
#include <cstdint>

namespace svf {

type U8 = uint8_t;
type U16 = uint16_t;
type U32 = uint32_t;
type U64 = uint64_t;

type I8 = int8_t;
type I16 = int16_t;
type I32 = int32_t;
type I64 = int64_t;

type F32 = float;
type F64 = double;

struct Array {
  uint32_t pointer_offset;
  uint32_t count;
};
  
namespace schema_format_0 {
#pragma pack(push, 1)

// forward declarations
struct EntryPoint;
struct ChoiceDefinition;
struct OptionDefinition;
struct StructDefinition;
struct FieldDefinition;
enum class ConcreteType_enum: U8;
union ConcreteType_union;
struct ConcreteType_dot_defined;
enum class Type_enum: U8;
union Type_union;
struct Type_dot_concrete;
struct Type_dot_static_array;
struct Type_dot_array;

// full declarations
struct EntryPoint {
  Array<StructDefinition_> structs;
  Array<ChoiceDefinition_> choices;
};

struct ChoiceDefinition {
  U64 name_hash;
  Array<OptionDefinition> options;
};

struct OptionDefinition {
  U64 name_hash;
  U16 index;
  Type_enum type_enum;
  U8 _padding_0[5];
  Type_union type_union;
};

struct StructDefinition {
  U64 name_hash;
  Array<FieldDefinition> fields;
};

struct FieldDefinition {
  U64 name_hash;
  Type_enum type_enum;
  U8 _padding_0[7];
  Type_union type_union;
};

enum class ConcreteType_enum: U8 {
  u8,
  u16,
  u32,
  u64,
  i8,
  i16,
  i32,
  i64,
  f32,
  f64,
  defined,
};

union ConcreteType_union {
  ConcreteType_dot_defined defined;
};

struct ConcreteType_dot_defined {
  U16 top_level_definition_index;
};

enum class Type_enum: U8 {
  concrete,
  fixed_size_array,
  array,
};

union Type_union {
  Type_dot_concrete concrete;
  Type_dot_pointer pointer;
  Type_dot_fixed_size_array fixed_size_array;
  Type_dot_flexible_array flexible_array;
};

struct Type_dot_pointer {
  ConcreteType_enum type_enum;
  U8 _pad0[7];
  ConcreteType_union type_union;
};

struct Type_dot_concrete {
  ConcreteType_enum type_enum;
  U8 _pad0[7];
  ConcreteType_union type_union;
};

struct Type_dot_fixed_size_array {
  U8 count;
  ConcreteType_enum element_type_enum;
  U8 _pad0[6];
  ConcreteType_union element_type_union;
};

struct Type_dot_flexible_array {
  ConcreteType_enum element_type_enum;
  U8 _pad0[7];
  ConcreteType_union element_type_union;
};

#pragma pack(pop)
} // namespace schema_format
} // namespace suf
```