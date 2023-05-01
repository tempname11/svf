#include "platform.hpp"

namespace grammar {

struct ConcreteType {
  enum class Which {
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
    unresolved,
  } which;

  struct Defined {
    U32 top_level_definition_index;
  };

  struct Unresolved {
    Range<Byte> name;
    U64 name_hash;
  };
  // This is only necessary before everything was parsed.
  // After that, a second pass must resolve all names.

  union {
    Defined defined;
    Unresolved unresolved;
  };
};

struct Type {
  enum class Which {
    concrete,
    pointer,
    flexible_array,
  } which;

  struct Concrete {
    ConcreteType type;
  };

  struct Pointer {
    ConcreteType type;
  };

  struct FlexibleArray {
    ConcreteType element_type;
  };

  union {
    Concrete concrete;
    Pointer pointer;
    FlexibleArray flexible_array;
  };
};

struct FieldDefinition {
  Range<Byte> name;
  U64 name_hash;
  Type type;
};

struct OptionDefinition {
  Range<Byte> name;
  U64 name_hash;
  U8 index;
  Type type;
};

struct StructDefinition {
  Range<Byte> name;
  U64 name_hash;
  Range<FieldDefinition> fields;
};

struct ChoiceDefinition {
  Range<Byte> name;
  U64 name_hash;
  Range<OptionDefinition> options;
};

struct TopLevelDefinition {
  enum class Which {
    a_struct,
    a_choice,
  } which;

  union {
    StructDefinition a_struct;
    ChoiceDefinition a_choice;
  };
};

struct Root {
  /*
  Range<Byte> schema_name;
  U32 entry_point_index;
  */
  Range<TopLevelDefinition> definitions;
};

struct OrderElement {
  size_t index;
  size_t ordering;
};

} // namespace grammar