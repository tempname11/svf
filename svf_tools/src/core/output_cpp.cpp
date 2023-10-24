#include <src/library.hpp>
#include <src/svf_runtime.h>
#include "../core.hpp"
#include "common.hpp"
#include "output_common.hpp"

namespace core::output::cpp {

void output_struct_declaration(Ctx ctx, meta::StructDefinition *it) {
  output_cstring(ctx, "struct ");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, ";\n");
}

void output_choice_declaration(Ctx ctx, meta::ChoiceDefinition *it) {
  output_cstring(ctx, "enum class ");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, "_tag: U8;\n");

  output_cstring(ctx, "union ");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, "_payload;\n");
}

void output_struct_index(Ctx ctx, svf::Sequence<U8> name, U32 index) {
  output_cstring(ctx, "U32 const ");
  output_u8_array(ctx, name);
  output_cstring(ctx, "_struct_index = ");
  output_decimal(ctx, index);
  output_cstring(ctx, ";\n");
}

void output_name_hash(Ctx ctx, svf::Sequence<U8> name, U64 hash) {
  output_cstring(ctx, "U64 const ");
  output_u8_array(ctx, name);
  output_cstring(ctx, "_name_hash = 0x");
  output_hexadecimal(ctx, hash);
  output_cstring(ctx, "ull;\n");
}

void output_concrete_type_name(
  Ctx ctx,
  meta::ConcreteType_tag in_tag,
  meta::ConcreteType_payload *in_payload
) {
  switch (in_tag) {
    case meta::ConcreteType_tag::definedStruct: {
      auto structs = to_range(ctx->schema_bytes, ctx->schema_definition->structs);
      output_u8_array(ctx, structs.pointer[in_payload->definedStruct.index].name);
      break;
    }
    case meta::ConcreteType_tag::definedChoice: {
      auto choices = to_range(ctx->schema_bytes, ctx->schema_definition->choices);
      output_u8_array(ctx, choices.pointer[in_payload->definedChoice.index].name);
      break;
    }
    case meta::ConcreteType_tag::u8: {
      output_cstring(ctx, "U8");
      break;
    }
    case meta::ConcreteType_tag::u16: {
      output_cstring(ctx, "U16");
      break;
    }
    case meta::ConcreteType_tag::u32: {
      output_cstring(ctx, "U32");
      break;
    }
    case meta::ConcreteType_tag::u64: {
      output_cstring(ctx, "U64");
      break;
    }
    case meta::ConcreteType_tag::i8: {
      output_cstring(ctx, "I8");
      break;
    }
    case meta::ConcreteType_tag::i16: {
      output_cstring(ctx, "I16");
      break;
    }
    case meta::ConcreteType_tag::i32: {
      output_cstring(ctx, "I32");
      break;
    }
    case meta::ConcreteType_tag::i64: {
      output_cstring(ctx, "I64");
      break;
    }
    case meta::ConcreteType_tag::f32: {
      output_cstring(ctx, "F32");
      break;
    }
    case meta::ConcreteType_tag::f64: {
      output_cstring(ctx, "F64");
      break;
    }
    default: {
      ASSERT(false);
    }
  }
}

void output_type(Ctx ctx, meta::Type_tag in_tag, meta::Type_payload *in_payload) {
  switch(in_tag) {
    case meta::Type_tag::concrete: {
      output_concrete_type_name(
        ctx,
        in_payload->concrete.type_tag,
        &in_payload->concrete.type_payload
      );
      break;
    }
    case meta::Type_tag::reference: {
      output_cstring(ctx, "Reference<");
      output_concrete_type_name(
        ctx,
        in_payload->concrete.type_tag,
        &in_payload->concrete.type_payload
      );
      output_cstring(ctx, ">");
      break;
    }
    case meta::Type_tag::sequence: {
      output_cstring(ctx, "Sequence<");
      output_concrete_type_name(
        ctx,
        in_payload->concrete.type_tag,
        &in_payload->concrete.type_payload
      );
      output_cstring(ctx, ">");
      break;
    }
    default: {
      ASSERT(false);
    }
  }
}

Bool output_struct(Ctx ctx, meta::StructDefinition *it) {
  auto structs = to_range(ctx->schema_bytes, ctx->schema_definition->structs);
  auto choices = to_range(ctx->schema_bytes, ctx->schema_definition->choices);

  output_cstring(ctx, "struct ");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, " {\n");

  UInt size_sum = 0;
  auto fields = to_range(ctx->schema_bytes, it->fields);


  for (UInt i = 0; i < fields.count; i++) {
    auto field = fields.pointer + i;

    // We don't support custom struct layouts yet.
    // TODO @proper-alignment.
    if (field->offset != size_sum) {
      return false;
    }

    auto plurality = get_plurality(
      structs,
      choices,
      field->type_tag,
      &field->type_payload
    );
    size_sum += plurality.size;

    switch (plurality.plurality) {
      case TypePlurality::zero: {
        continue;
      }
      case TypePlurality::one: {
        output_cstring(ctx, "  ");
        output_type(ctx, field->type_tag, &field->type_payload);
        output_cstring(ctx, " ");
        output_u8_array(ctx, field->name);
        output_cstring(ctx, ";\n");
        break;
      }
      case TypePlurality::tag_and_payload: {
        if (field->type_tag != meta::Type_tag::concrete) {
          return false;
        }

        output_cstring(ctx, "  ");
        output_concrete_type_name(
          ctx,
          field->type_payload.concrete.type_tag,
          &field->type_payload.concrete.type_payload
        );
        output_cstring(ctx, "_tag ");
        output_u8_array(ctx, field->name);
        output_cstring(ctx, "_tag;\n  ");
        output_concrete_type_name(
          ctx,
          field->type_payload.concrete.type_tag,
          &field->type_payload.concrete.type_payload
        );
        output_cstring(ctx, "_payload ");
        output_u8_array(ctx, field->name);
        output_cstring(ctx, "_payload;\n");
        break;
      }
      default: {
        return false;
      }
    }
  }

  // TODO @proper-alignment.
  if (size_sum != it->size) {
    return false;
  }

  output_cstring(ctx, "};\n\n");
  return true;
}

Bool output_choice(Ctx ctx, meta::ChoiceDefinition *it) {
  auto structs = to_range(ctx->schema_bytes, ctx->schema_definition->structs);
  auto choices = to_range(ctx->schema_bytes, ctx->schema_definition->choices);

  output_cstring(ctx, "enum class ");
  output_u8_array(ctx, it->name);
  static_assert(SVFRT_TAG_SIZE == 1);
  output_cstring(ctx, "_tag: U8 {\n");

  auto options = to_range(ctx->schema_bytes, it->options);
  UInt size_max = 0;

  for (UInt i = 0; i < options.count; i++) {
    auto option = options.pointer + i;
    output_cstring(ctx, "  ");
    output_u8_array(ctx, option->name);
    output_cstring(ctx, " = ");
    output_decimal(ctx, (U64) option->tag);
    output_cstring(ctx, ",\n");
  }
  output_cstring(ctx, "};\n\n");

  output_cstring(ctx, "union ");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, "_payload {\n");
  for (UInt i = 0; i < options.count; i++) {
    auto option = options.pointer + i;

    auto plurality = get_plurality(
      structs,
      choices,
      option->type_tag,
      &option->type_payload
    );
    size_max = maxi(size_max, plurality.size);

    if (plurality.plurality == TypePlurality::zero) {
      continue;
    }
    if (plurality.plurality != TypePlurality::one) {
      return false;
    }

    output_cstring(ctx, "  ");
    output_type(ctx, option->type_tag, &option->type_payload);
    output_cstring(ctx, " ");
    output_u8_array(ctx, option->name);
    output_cstring(ctx, ";\n");
  }

  if (size_max != it->payloadSize) {
    return false;
  }

  output_cstring(ctx, "};\n\n");
  return true;
}

char const *header = R"(// AUTOGENERATED by svfc.
#pragma once
#include <cstdint>
#include <cstddef>

namespace svf {

#ifndef SVF_COMMON_CPP_TYPES_INCLUDED
#define SVF_COMMON_CPP_TYPES_INCLUDED

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

#pragma pack(push, 1)

template<typename T>
struct Reference {
  U32 data_offset_complement;
};

template<typename T>
struct Sequence {
  U32 data_offset_complement;
  U32 count;
};

#pragma pack(pop)
#endif // SVF_COMMON_CPP_TYPES_INCLUDED

#ifndef SVF_COMMON_CPP_TRICKERY_INCLUDED
#define SVF_COMMON_CPP_TRICKERY_INCLUDED

template<typename T>
struct GetSchemaFromType;

#endif // SVF_COMMON_CPP_TRICKERY_INCLUDED

)";


Bytes as_code(
  vm::LinearArena *arena,
  Bytes schema_bytes,
  validation::Result *validation_result
) {
  // TODO @proper-alignment.
  auto schema_definition = (meta::SchemaDefinition *) (
    schema_bytes.pointer +
    schema_bytes.count -
    sizeof(meta::SchemaDefinition)
  );

  auto start = vm::realign(arena);
  OutputContext context_value = {
    .dedicated_arena = arena,
    .schema_definition = schema_definition,
    .schema_bytes = schema_bytes,
  };

  auto ctx = &context_value;
  auto structs = to_range(schema_bytes, schema_definition->structs);
  auto choices = to_range(schema_bytes, schema_definition->choices);

  output_cstring(ctx, header);

  output_cstring(ctx, "namespace ");
  output_u8_array(ctx, schema_definition->name);
  output_cstring(ctx, " {\n");

  output_cstring(ctx, "#pragma pack(push, 1)\n");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "extern U32 const struct_strides[];\n");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "namespace binary {\n");
  output_cstring(ctx, "  size_t const size = ");
  output_decimal(ctx, schema_bytes.count);
  output_cstring(ctx, ";\n");

  output_cstring(ctx, "  extern U8 const array[];\n");
  output_cstring(ctx, "} // namespace binary\n");

  output_cstring(ctx, "\n// Forward declarations.\n");
  for (UInt i = 0; i < structs.count; i++) {
    auto it = structs.pointer + i;
    output_struct_declaration(ctx, it);
  }
  for (UInt i = 0; i < choices.count; i++) {
    auto it = choices.pointer + i;
    output_choice_declaration(ctx, it);
  }

  output_cstring(ctx, "\n// Indexes of structs.\n");
  for (UInt i = 0; i < structs.count; i++) {
    auto it = structs.pointer + i;
    output_struct_index(ctx, it->name, i);
  }

  output_cstring(ctx, "\n// Hashes of top level definition names.\n");
  for (UInt i = 0; i < structs.count; i++) {
    auto it = structs.pointer + i;
    output_name_hash(ctx, it->name, it->nameHash);
  }
  for (UInt i = 0; i < choices.count; i++) {
    auto it = choices.pointer + i;
    output_name_hash(ctx, it->name, it->nameHash);
  }

  output_cstring(ctx, "\n// Full declarations.\n");

  for (UInt i = 0; i < validation_result->ordering.count; i++) {
    auto item = validation_result->ordering.pointer + i;

    if (item->type == meta::ConcreteType_tag::definedStruct) {
      if (!output_struct(ctx, structs.pointer + item->index)) {
        return {};
      }
    } else if (item->type == meta::ConcreteType_tag::definedChoice) {
      if (!output_choice(ctx, choices.pointer + item->index)) {
        return {};
      }
    } else {
      return {};
    }
  }

  output_cstring(ctx, "#pragma pack(pop)\n\n");

  output_cstring(ctx, "// C++ trickery: SchemaDescription.\n");
  output_cstring(ctx, R"(struct SchemaDescription {
  template<typename T>
  struct PerType;

  static constexpr U32 *schema_struct_strides = (U32 *) struct_strides;
  static constexpr U8 *schema_binary_array = (U8 *) binary::array;
  static constexpr size_t schema_binary_size = binary::size;)");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "  static constexpr U32 schema_struct_count = ");
  output_decimal(ctx, structs.count);
  output_cstring(ctx, ";\n");

  output_cstring(ctx, "  static constexpr U32 min_read_scratch_memory_size = ");
  output_decimal(ctx, get_min_read_scratch_memory_size(schema_definition));
  output_cstring(ctx, ";\n");

  output_cstring(ctx, "  static constexpr U32 compatibility_work_base = ");
  output_decimal(ctx, get_compatibility_work_base(schema_bytes));
  output_cstring(ctx, ";\n");

  output_cstring(ctx, "  static constexpr U64 name_hash = 0x");
  output_hexadecimal(ctx, schema_definition->nameHash);
  output_cstring(ctx, "ull;\n");

  output_cstring(ctx, "  static constexpr U64 content_hash = 0x");
  output_hexadecimal(ctx, get_content_hash(schema_bytes));
  output_cstring(ctx, "ull;\n");

  output_cstring(ctx, "};\n");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "// C++ trickery: SchemaDescription::PerType.\n");

  for (UInt i = 0; i < structs.count; i++) {
    auto it = structs.pointer + i;
    output_cstring(ctx, "template<>\nstruct SchemaDescription::PerType<");
    output_u8_array(ctx, it->name);
    output_cstring(ctx, "> {\n  static constexpr U64 name_hash = ");
    output_u8_array(ctx, it->name);
    output_cstring(ctx, "_name_hash;\n  static constexpr U32 index = ");
    output_u8_array(ctx, it->name);
    output_cstring(ctx, "_struct_index;\n");
    output_cstring(ctx, "};\n\n");
  }

  output_cstring(ctx, "} // namespace ");
  output_u8_array(ctx, schema_definition->name);
  output_cstring(ctx, "\n\n");

  output_cstring(ctx, "// C++ trickery: GetSchemaFromType.\n");
  for (UInt i = 0; i < structs.count; i++) {
    auto it = structs.pointer + i;
    output_cstring(ctx, "template<>\nstruct GetSchemaFromType<");
    output_u8_array(ctx, schema_definition->name);
    output_cstring(ctx, "::");
    output_u8_array(ctx, it->name);
    output_cstring(ctx, "> {\n  using SchemaDescription = ");
    output_u8_array(ctx, schema_definition->name);
    output_cstring(ctx, "::SchemaDescription;\n};\n\n");
  }

  // Note: ".h" and ".hpp" generated files currently embed the binary schema
  // separately, and if a program uses both for some reason, it will have to
  // link two copies. This is a bit weird, but unlikely and relatively harmless.

  output_cstring(ctx, "// Binary schema.\n");
  output_cstring(ctx, "#if defined(SVF_INCLUDE_BINARY_SCHEMA) || defined(SVF_IMPLEMENTATION)\n");
  output_cstring(ctx, "#ifndef SVF_");
  output_u8_array(ctx, schema_definition->name);
  output_cstring(ctx, "_BINARY_INCLUDED_HPP\n");

  output_cstring(ctx, "namespace ");
  output_u8_array(ctx, schema_definition->name);
  output_cstring(ctx, " {\n");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "U32 const struct_strides[] = {\n");
  for (UInt i = 0; i < structs.count; i++) {
    auto it = structs.pointer + i;
    if (i != 0) {
      output_cstring(ctx, ",\n");
    }
    output_cstring(ctx, "  ");
    output_decimal(ctx, it->size);
  }
  output_cstring(ctx, "\n");
  output_cstring(ctx, "};\n");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "namespace binary {\n");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "U8 const array[] = {\n");
  output_raw_bytes(ctx, schema_bytes);
  output_cstring(ctx, "};\n");

  output_cstring(ctx, "\n");
  output_cstring(ctx, "} // namespace binary\n");
  output_cstring(ctx, "} // namespace ");
  output_u8_array(ctx, schema_definition->name);
  output_cstring(ctx, "\n");

  output_cstring(ctx, "#endif // SVF_");
  output_u8_array(ctx, schema_definition->name);
  output_cstring(ctx, "_BINARY_INCLUDED_HPP\n");
  output_cstring(ctx, "#endif // defined(SVF_INCLUDE_BINARY_SCHEMA) || defined(SVF_IMPLEMENTATION)\n");

  output_cstring(ctx, "\n");
  output_cstring(ctx, "} // namespace svf\n");

  auto end = arena->reserved_range.pointer + arena->waterline;
  return {
    .pointer = (Byte *) start,
    .count = offset_between<U64>(start, end),
  };
}

} // namespace output::cpp
