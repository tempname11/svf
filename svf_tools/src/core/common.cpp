#include <src/svf_runtime.h>
#include "common.hpp"

TypePluralityAndSize get_plurality(
  Range<svf::Meta::StructDefinition> structs,
  Range<svf::Meta::ChoiceDefinition> choices,
  svf::Meta::Type_tag in_tag,
  svf::Meta::Type_payload *in_payload
) {
  namespace meta = svf::Meta;

  switch (in_tag) {
    case meta::Type_tag::concrete: {
      switch (in_payload->concrete.type_tag) {
        case meta::ConcreteType_tag::u8: {
          return { TypePlurality::one, 1 };
        }
        case meta::ConcreteType_tag::u16: {
          return { TypePlurality::one, 2 };
        }
        case meta::ConcreteType_tag::u32: {
          return { TypePlurality::one, 4 };
        }
        case meta::ConcreteType_tag::u64: {
          return { TypePlurality::one, 8 };
        }
        case meta::ConcreteType_tag::i8: {
          return { TypePlurality::one, 1 };
        }
        case meta::ConcreteType_tag::i16: {
          return { TypePlurality::one, 2 };
        }
        case meta::ConcreteType_tag::i32: {
          return { TypePlurality::one, 4 };
        }
        case meta::ConcreteType_tag::i64: {
          return { TypePlurality::one, 8 };
        }
        case meta::ConcreteType_tag::f32: {
          return { TypePlurality::one, 4 };
        }
        case meta::ConcreteType_tag::f64: {
          return { TypePlurality::one, 8 };
        }
        case meta::ConcreteType_tag::zeroSized: {
          return { TypePlurality::zero, 0 };
        }
        case meta::ConcreteType_tag::definedStruct: {
          auto index = in_payload->concrete.type_payload.definedStruct.index;
          ASSERT(index < structs.count);
          auto size = structs.pointer[index].size;
          return { TypePlurality::one, size };
        }
        case meta::ConcreteType_tag::definedChoice: {
          auto index = in_payload->concrete.type_payload.definedChoice.index;
          ASSERT(index < choices.count);
          auto payload_size = choices.pointer[index].payloadSize;
          // TODO @proper-alignment.
          return { TypePlurality::tag_and_payload, SVFRT_TAG_SIZE + payload_size };
        }
        default: {
          ASSERT(false);
          return {};
        }
      }
    }
    case meta::Type_tag::reference: {
      return { TypePlurality::one, 4 };
    }
    case meta::Type_tag::sequence: {
      return { TypePlurality::one, 8 };
    }
    default: {
      ASSERT(false);
      return {};
    }
  }
}
