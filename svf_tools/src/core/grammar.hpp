#pragma once
#include <src/library.hpp>

namespace core::grammar {

// Note: this was written before `meta.hpp` could be generated, and they are
// largely the same. The main difference is that this file is written in a
// more "human readable" way, while `meta.hpp` is generated by a script.
//
// TODO: we don't really need both, but there will be some quirks when using the
// `meta.hpp` declarations for parsing, for instance:
// - Struct/choice indices are not yet known, and will need to refer to names instead.
// - Binary sizes/offsets are not yet known and will be unused.

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
