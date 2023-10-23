#pragma once
#include <src/library.hpp>
#include <src/svf_meta.hpp>
#include "core/grammar.hpp"

namespace core {
  namespace meta = svf::META;

  namespace parsing {
    grammar::Root *parse_input(vm::LinearArena *arena, Range<U8> input);
  }

  namespace generation {
    Bytes as_bytes(
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
