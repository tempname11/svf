#include "common.hpp"

TypePluralityAndSize get_plurality(
  Range<svf::META::StructDefinition> structs,
  Range<svf::META::ChoiceDefinition> choices,
  svf::META::Type_enum in_enum,
  svf::META::Type_union *in_union
) {
  namespace meta = svf::META;

  switch (in_enum) {
    case meta::Type_enum::concrete: {
      switch (in_union->concrete.type_enum) {
        case meta::ConcreteType_enum::u8: {
          return { TypePlurality::one, 1 };
        }
        case meta::ConcreteType_enum::u16: {
          return { TypePlurality::one, 2 };
        }
        case meta::ConcreteType_enum::u32: {
          return { TypePlurality::one, 4 };
        }
        case meta::ConcreteType_enum::u64: {
          return { TypePlurality::one, 8 };
        }
        case meta::ConcreteType_enum::i8: {
          return { TypePlurality::one, 1 };
        }
        case meta::ConcreteType_enum::i16: {
          return { TypePlurality::one, 2 };
        }
        case meta::ConcreteType_enum::i32: {
          return { TypePlurality::one, 4 };
        }
        case meta::ConcreteType_enum::i64: {
          return { TypePlurality::one, 8 };
        }
        case meta::ConcreteType_enum::f32: {
          return { TypePlurality::one, 4 };
        }
        case meta::ConcreteType_enum::f64: {
          return { TypePlurality::one, 8 };
        }
        case meta::ConcreteType_enum::zero_sized: {
          return { TypePlurality::zero, 0 };
        }
        case meta::ConcreteType_enum::defined_struct: {
          auto index = in_union->concrete.type_union.defined_struct.index;
          ASSERT(index < structs.count);
          auto size = structs.pointer[index].size;
          return { TypePlurality::one, size };
        }
        case meta::ConcreteType_enum::defined_choice: {
          auto index = in_union->concrete.type_union.defined_choice.index;
          ASSERT(index < choices.count);
          auto payload_size = choices.pointer[index].payload_size;
          U32 tag_size = 1; // Only U8 supported right now. @only-u8-tag
          return { TypePlurality::enum_and_union, tag_size + payload_size };
        }
        default: {
          ASSERT(false);
          return {};
        }
      }
    }
    case meta::Type_enum::pointer: {
      return { TypePlurality::one, 4 };
    }
    case meta::Type_enum::flexible_array: {
      return { TypePlurality::one, 8 };
    }
    default: {
      ASSERT(false);
      return {};
    }
  }
}
