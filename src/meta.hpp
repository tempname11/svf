#include <cstdint>

namespace svf {

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

template<typename T>
struct Array {
  uint32_t pointer_offset;
  uint32_t count;
};
  
namespace schema_format_0 {
#pragma pack(push, 1)

// Forward declarations
struct EntryPoint;
struct ChoiceDefinition;
struct ConcreteType_dot_defined;
struct StructDefinition;
enum class ConcreteType_enum;
union ConcreteType_union;
struct Type_dot_flexible_array;
struct Type_dot_concrete;
struct Type_dot_pointer;
enum class Type_enum;
union Type_union;
struct OptionDefinition;
struct FieldDefinition;

// Full declarations
struct EntryPoint {
  Array<StructDefinition> structs;
  Array<ChoiceDefinition> choices;
};

struct ChoiceDefinition {
  Array<OptionDefinition> options;
};

struct ConcreteType_dot_defined {
  U32 top_level_definition_index;
};

struct StructDefinition {
  Array<FieldDefinition> fields;
};

enum class ConcreteType_enum {
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
  zero_sized,
  defined,
};

union ConcreteType_union {
  ConcreteType_dot_defined defined;
};

struct Type_dot_flexible_array {
  ConcreteType_enum element_type_enum;
  ConcreteType_union element_type_union;
};

struct Type_dot_concrete {
  ConcreteType_enum type_enum;
  ConcreteType_union type_union;
};

struct Type_dot_pointer {
  ConcreteType_enum type_enum;
  ConcreteType_union type_union;
};

enum class Type_enum {
  concrete,
  pointer,
  flexible_array,
};

union Type_union {
  Type_dot_concrete concrete;
  Type_dot_pointer pointer;
  Type_dot_flexible_array flexible_array;
};

struct OptionDefinition {
  U64 name_hash;
  U8 index;
  Type_enum type_enum;
  Type_union type_union;
};

struct FieldDefinition {
  U64 name_hash;
  Type_enum type_enum;
  Type_union type_union;
};

#pragma pack(pop)
} // namespace schema_format_0

} // namespace svf

