#include <src/svf_runtime.h>
#include "common.hpp"

namespace core {

TypePluralityAndSize get_plurality(
  Range<svf::Meta::StructDefinition> structs,
  Range<svf::Meta::ChoiceDefinition> choices,
  svf::Meta::Type_tag in_tag,
  svf::Meta::Type_payload *in_payload
) {
  switch (in_tag) {
    case Meta::Type_tag::concrete: {
      switch (in_payload->concrete.type_tag) {
        case Meta::ConcreteType_tag::u8: {
          return { TypePlurality::one, 1 };
        }
        case Meta::ConcreteType_tag::u16: {
          return { TypePlurality::one, 2 };
        }
        case Meta::ConcreteType_tag::u32: {
          return { TypePlurality::one, 4 };
        }
        case Meta::ConcreteType_tag::u64: {
          return { TypePlurality::one, 8 };
        }
        case Meta::ConcreteType_tag::i8: {
          return { TypePlurality::one, 1 };
        }
        case Meta::ConcreteType_tag::i16: {
          return { TypePlurality::one, 2 };
        }
        case Meta::ConcreteType_tag::i32: {
          return { TypePlurality::one, 4 };
        }
        case Meta::ConcreteType_tag::i64: {
          return { TypePlurality::one, 8 };
        }
        case Meta::ConcreteType_tag::f32: {
          return { TypePlurality::one, 4 };
        }
        case Meta::ConcreteType_tag::f64: {
          return { TypePlurality::one, 8 };
        }
        case Meta::ConcreteType_tag::zeroSized: {
          return { TypePlurality::zero, 0 };
        }
        case Meta::ConcreteType_tag::definedStruct: {
          auto index = in_payload->concrete.type_payload.definedStruct.index;
          ASSERT(index < structs.count);
          auto size = structs.pointer[index].size;
          return { TypePlurality::one, size };
        }
        case Meta::ConcreteType_tag::definedChoice: {
          auto index = in_payload->concrete.type_payload.definedChoice.index;
          ASSERT(index < choices.count);
          auto payload_size = choices.pointer[index].payloadSize;
          // TODO @proper-alignment: tags.
          return { TypePlurality::tag_and_payload, SVFRT_TAG_SIZE + payload_size };
        }
        default: {
          return UNREACHABLE;
        }
      }
    }
    case Meta::Type_tag::reference: {
      return { TypePlurality::one, 4 };
    }
    case Meta::Type_tag::sequence: {
      return { TypePlurality::one, 8 };
    }
    default: {
      return UNREACHABLE;
    }
  }
}

} // namespace core
