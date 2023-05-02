#include "svf/grammar.hpp"

namespace svf {
  namespace output_binary {
    Range<Byte> output_bytes(
      grammar::Root *root,
      Range<grammar::OrderElement> ordering,
      vm::LinearArena *arena,
      vm::LinearArena *output_arena
    );
  }

  namespace output_cpp {
    Range<Byte> output_code(
      grammar::Root *root,
      Range<grammar::OrderElement> ordering,
      vm::LinearArena *arena,
      vm::LinearArena *output_arena
    );
  }

  namespace parsing {
    grammar::Root *parse_input(vm::LinearArena *arena, Range<U8> input);
  }

  namespace typechecking {
    bool resolve_types(grammar::Root *root);
    Range<grammar::OrderElement> order_types(
      grammar::Root *root,
      vm::LinearArena *arena
    );
  }
}