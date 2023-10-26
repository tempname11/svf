#pragma once
#include <src/library.hpp>
#include <src/svf_meta.hpp>
#include "core/grammar.hpp"

namespace core {
  namespace meta = svf::Meta;

  namespace parsing {
    enum class FailCode {
      ok = 0,
      expected_newline                                                   = 0x01,
      expected_name_directive                                            = 0x02,
      expected_uppercase_letter                                          = 0x03,
      expected_lowercase_letter                                          = 0x04,
      expected_type_definition                                           = 0x05,
      expected_semicolon                                                 = 0x06,
      expected_colon_or_semicolon                                        = 0x07,
      expected_colon_after_type_name                                     = 0x08,
      expected_colon_after_field_name                                    = 0x09,
      expected_opening_curly_bracket                                     = 0x0A,
      expected_closing_square_bracket                                    = 0x0B,
      keyword_reserved                                                   = 0x0C,
      backtrack                                                          = 0xFF,
    };

    struct Fail {
      FailCode code;
      U64 cursor;
    };

    struct ParseResult {
      grammar::Root *root;
      Fail fail;
    };

    ParseResult parse_input(vm::LinearArena *arena, Range<U8> input);

    Range<U8> get_fail_code_description(FailCode code);
  }

  namespace generation {
    enum class FailCode {
      ok = 0,
      type_not_found                                                     = 0x01,
      cyclical_dependency                                                = 0x02,
      empty_struct                                                       = 0x03,
      empty_choice                                                       = 0x04,
      choice_not_allowed                                                 = 0x05,
    };

    struct GenerationResult {
      Bytes schema;
      FailCode fail_code;
    };

    GenerationResult as_bytes(
      grammar::Root *root,
      vm::LinearArena *arena
    );
  }

  namespace validation {
    struct TLDRef {
      UInt index;
      meta::ConcreteType_tag type; // Must be a struct or a choice.
    };

    struct Result {
      bool valid;
      Range<TLDRef> ordering;
    };

    Result validate(vm::LinearArena *arena, Bytes schema_bytes);
  }

  namespace output::cpp {
    Range<U8> as_code(
      vm::LinearArena *arena,
      Bytes schema_bytes,
      validation::Result *validation_result
    );
  }

  namespace output::c {
    Range<U8> as_code(
      vm::LinearArena *arena,
      Bytes schema_bytes,
      validation::Result *validation_result
    );
  }

  namespace compatiblity::binary {
    struct Result {
      bool valid;
      Range<U32> struct_strides;
    };
    Result check_entry(
      Bytes scratch_memory,
      Bytes schema_write,
      Bytes schema_read,
      U64 entry_struct_name_hash
    );
  }
}
