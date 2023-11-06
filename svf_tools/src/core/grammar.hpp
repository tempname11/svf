#pragma once
#include <src/library.hpp>

namespace core::grammar {

// This looks a bit like the "meta.hpp" metaschema, but this is specific for
// parsing, and differs from those definitions in a few ways:
//
// - Struct/choice indices are not yet known, and name hashes are used instead.
// - Binary sizes/offsets are not yet known.
// - Pointers are used directly.
// - Names are referenced directly.
//

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
  } which;

  // This struct is used to represent a type that is defined elsewhere.
  // At the time of parsing, the definition is not yet resolved.
  struct Defined {
    U64 top_level_definition_name_hash;
  };

  union {
    Defined defined;
  };
};

struct Type {
  enum class Which {
    concrete,
    reference,
    sequence,
  } which;

  struct Concrete {
    ConcreteType type;
  };

  struct Reference {
    ConcreteType type;
  };

  struct Sequence {
    ConcreteType element_type;
  };

  union {
    Concrete concrete;
    Reference reference;
    Sequence sequence;
  };
};

struct FieldDefinition {
  Range<U8> name;
  U64 name_hash; // Now called "fieldId" in the metaschema.
  Type type;
};

struct OptionDefinition {
  Range<U8> name;
  U64 name_hash; // Now called "optionId" in the metaschema.
  Type type;
};

struct StructDefinition {
  Range<U8> name;
  U64 name_hash; // Now called "typeId" in the metaschema.
  Range<FieldDefinition> fields;
};

struct ChoiceDefinition {
  Range<U8> name;
  U64 name_hash; // Now called "typeId" in the metaschema.
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
  Range<U8> schema_name;
  U64 schema_name_hash; // Now called "schemaId" in the metaschema.
  Range<TopLevelDefinition> definitions;
};

} // namespace grammar
