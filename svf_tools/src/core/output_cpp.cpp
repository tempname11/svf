#include <src/library.hpp>
#include <src/svf_runtime.h>
#include "../core.hpp"
#include "common.hpp"
#include "output_common.hpp"

namespace core::output::cpp {

void output_struct_declaration(Ctx ctx, Meta::StructDefinition *it) {
  output_cstring(ctx, "struct ");
  output_name(ctx, it->typeId);
  output_cstring(ctx, ";\n");
}

void output_choice_declaration(Ctx ctx, Meta::ChoiceDefinition *it) {
  output_cstring(ctx, "enum class ");
  output_name(ctx, it->typeId);
  output_cstring(ctx, "_tag: uint8_t;\n");

  output_cstring(ctx, "union ");
  output_name(ctx, it->typeId);
  output_cstring(ctx, "_payload;\n");
}

void output_struct_index(Ctx ctx, U64 type_id, U32 index) {
  output_cstring(ctx, "uint32_t const ");
  output_name(ctx, type_id);
  output_cstring(ctx, "_struct_index = ");
  output_decimal(ctx, index);
  output_cstring(ctx, ";\n");
}

void output_type_id(Ctx ctx, U64 type_id) {
  output_cstring(ctx, "uint64_t const ");
  output_name(ctx, type_id);
  output_cstring(ctx, "_type_id = 0x");
  output_hexadecimal(ctx, type_id);
  output_cstring(ctx, "ull;\n");
}

void output_concrete_type_name(
  Ctx ctx,
  Meta::ConcreteType_tag in_tag,
  Meta::ConcreteType_payload *in_payload
) {
  switch (in_tag) {
    case Meta::ConcreteType_tag::definedStruct: {
      auto structs = to_range(ctx->schema_bytes, ctx->schema_definition->structs);
      output_name(ctx, structs.pointer[in_payload->definedStruct.index].typeId);
      break;
    }
    case Meta::ConcreteType_tag::definedChoice: {
      auto choices = to_range(ctx->schema_bytes, ctx->schema_definition->choices);
      output_name(ctx, choices.pointer[in_payload->definedChoice.index].typeId);
      break;
    }
    case Meta::ConcreteType_tag::u8: {
      output_cstring(ctx, "uint8_t");
      break;
    }
    case Meta::ConcreteType_tag::u16: {
      output_cstring(ctx, "uint16_t");
      break;
    }
    case Meta::ConcreteType_tag::u32: {
      output_cstring(ctx, "uint32_t");
      break;
    }
    case Meta::ConcreteType_tag::u64: {
      output_cstring(ctx, "uint64_t");
      break;
    }
    case Meta::ConcreteType_tag::i8: {
      output_cstring(ctx, "int8_t");
      break;
    }
    case Meta::ConcreteType_tag::i16: {
      output_cstring(ctx, "int16_t");
      break;
    }
    case Meta::ConcreteType_tag::i32: {
      output_cstring(ctx, "int32_t");
      break;
    }
    case Meta::ConcreteType_tag::i64: {
      output_cstring(ctx, "int64_t");
      break;
    }
    case Meta::ConcreteType_tag::f32: {
      output_cstring(ctx, "float");
      break;
    }
    case Meta::ConcreteType_tag::f64: {
      output_cstring(ctx, "double");
      break;
    }
    default: {
      UNREACHABLE;
    }
  }
}

void output_type(Ctx ctx, Meta::Type_tag in_tag, Meta::Type_payload *in_payload) {
  switch(in_tag) {
    case Meta::Type_tag::concrete: {
      output_concrete_type_name(
        ctx,
        in_payload->concrete.type_tag,
        &in_payload->concrete.type_payload
      );
      break;
    }
    case Meta::Type_tag::reference: {
      output_cstring(ctx, "runtime::Reference<");
      output_concrete_type_name(
        ctx,
        in_payload->concrete.type_tag,
        &in_payload->concrete.type_payload
      );
      output_cstring(ctx, ">");
      break;
    }
    case Meta::Type_tag::sequence: {
      output_cstring(ctx, "runtime::Sequence<");
      output_concrete_type_name(
        ctx,
        in_payload->concrete.type_tag,
        &in_payload->concrete.type_payload
      );
      output_cstring(ctx, ">");
      break;
    }
    default: {
      UNREACHABLE;
    }
  }
}

Bool output_struct(Ctx ctx, Meta::StructDefinition *it) {
  auto structs = to_range(ctx->schema_bytes, ctx->schema_definition->structs);
  auto choices = to_range(ctx->schema_bytes, ctx->schema_definition->choices);

  output_cstring(ctx, "struct ");
  output_name(ctx, it->typeId);
  output_cstring(ctx, " {\n");

  UInt size_sum = 0;
  auto fields = to_range(ctx->schema_bytes, it->fields);


  for (UInt i = 0; i < fields.count; i++) {
    auto field = fields.pointer + i;

    // We don't support custom struct layouts yet.
    if (field->offset != size_sum) {
      return false;
    }

    auto plurality = get_plurality(
      structs,
      choices,
      field->type_tag,
      &field->type_payload
    );

    // TODO @proper-alignment: align up first.
    size_sum += plurality.size;

    switch (plurality.plurality) {
      case TypePlurality::zero: {
        continue;
      }
      case TypePlurality::one: {
        output_cstring(ctx, "  ");
        output_type(ctx, field->type_tag, &field->type_payload);
        output_cstring(ctx, " ");
        output_name(ctx, field->fieldId);
        output_cstring(ctx, ";\n");
        break;
      }
      case TypePlurality::tag_and_payload: {
        if (field->type_tag != Meta::Type_tag::concrete) {
          return false;
        }

        output_cstring(ctx, "  ");
        output_concrete_type_name(
          ctx,
          field->type_payload.concrete.type_tag,
          &field->type_payload.concrete.type_payload
        );
        output_cstring(ctx, "_tag ");
        output_name(ctx, field->fieldId);
        output_cstring(ctx, "_tag;\n  ");
        output_concrete_type_name(
          ctx,
          field->type_payload.concrete.type_tag,
          &field->type_payload.concrete.type_payload
        );
        output_cstring(ctx, "_payload ");
        output_name(ctx, field->fieldId);
        output_cstring(ctx, "_payload;\n");
        break;
      }
      default: {
        return false;
      }
    }
  }

  if (size_sum != it->size) {
    return false;
  }

  output_cstring(ctx, "};\n\n");
  return true;
}

Bool output_choice(Ctx ctx, Meta::ChoiceDefinition *it) {
  auto structs = to_range(ctx->schema_bytes, ctx->schema_definition->structs);
  auto choices = to_range(ctx->schema_bytes, ctx->schema_definition->choices);

  output_cstring(ctx, "enum class ");
  output_name(ctx, it->typeId);
  static_assert(SVFRT_TAG_SIZE == 1);
  output_cstring(ctx, "_tag: uint8_t {\n");

  auto options = to_range(ctx->schema_bytes, it->options);
  UInt size_max = 0;

  output_cstring(ctx, "  nothing = 0,\n");

  for (UInt i = 0; i < options.count; i++) {
    auto option = options.pointer + i;
    output_cstring(ctx, "  ");
    output_name(ctx, option->optionId);
    output_cstring(ctx, " = ");
    output_decimal(ctx, (U64) option->tag);
    output_cstring(ctx, ",\n");
  }
  output_cstring(ctx, "};\n\n");

  output_cstring(ctx, "union ");
  output_name(ctx, it->typeId);
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
    output_name(ctx, option->optionId);
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
#pragma pack(push, 1)
namespace runtime {

template<typename T>
struct Reference {
  uint32_t data_offset_complement;
};

template<typename T>
struct Sequence {
  uint32_t data_offset_complement;
  uint32_t count;
};

template<typename T> struct GetSchemaFromType;

} // namespace runtime
#pragma pack(pop)
#endif // SVF_COMMON_CPP_TYPES_INCLUDED

)";

Bytes as_code(
  vm::LinearArena *arena,
  Bytes schema_bytes,
  Bytes appendix_bytes,
  validation::Result *validation_result
) {
  // TODO @proper-alignment: struct access.
  auto schema_definition = (Meta::SchemaDefinition *) (
    schema_bytes.pointer +
    schema_bytes.count -
    sizeof(Meta::SchemaDefinition)
  );

  // TODO @proper-alignment: struct access.
  auto appendix = (Meta::Appendix *) (
    appendix_bytes.pointer +
    appendix_bytes.count -
    sizeof(Meta::Appendix)
  );

  auto start = vm::realign(arena);
  OutputContext context_value = {
    .dedicated_arena = arena,
    .schema_definition = schema_definition,
    .schema_bytes = schema_bytes,
    .appendix = appendix,
    .appendix_bytes = appendix_bytes,
  };

  auto ctx = &context_value;
  auto structs = to_range(schema_bytes, schema_definition->structs);
  auto choices = to_range(schema_bytes, schema_definition->choices);

  output_cstring(ctx, header);

  output_cstring(ctx, "namespace ");
  output_name(ctx, schema_definition->schemaId);
  output_cstring(ctx, " {\n");

  output_cstring(ctx, "#pragma pack(push, 1)\n");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "extern uint32_t const struct_strides[];\n");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "namespace binary {\n");
  output_cstring(ctx, "  size_t const size = ");
  output_decimal(ctx, schema_bytes.count);
  output_cstring(ctx, ";\n");

  output_cstring(ctx, "  extern uint8_t const array[];\n");
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
    output_struct_index(ctx, it->typeId, i);
  }

  output_cstring(ctx, "\n// Hashes of top level definition names.\n");
  for (UInt i = 0; i < structs.count; i++) {
    auto it = structs.pointer + i;
    output_type_id(ctx, it->typeId);
  }
  for (UInt i = 0; i < choices.count; i++) {
    auto it = choices.pointer + i;
    output_type_id(ctx, it->typeId);
  }

  output_cstring(ctx, "\n// Full declarations.\n");

  for (UInt i = 0; i < validation_result->ordering.count; i++) {
    auto item = validation_result->ordering.pointer + i;

    if (item->type == Meta::ConcreteType_tag::definedStruct) {
      if (!output_struct(ctx, structs.pointer + item->index)) {
        return {};
      }
    } else if (item->type == Meta::ConcreteType_tag::definedChoice) {
      if (!output_choice(ctx, choices.pointer + item->index)) {
        return {};
      }
    } else {
      return {};
    }
  }

  output_cstring(ctx, "#pragma pack(pop)\n\n");

  output_cstring(ctx, "// C++ trickery: _SchemaDescription.\n");
  output_cstring(ctx, R"(struct _SchemaDescription {
  template<typename T>
  struct PerType;

  static constexpr uint32_t *schema_struct_strides = (uint32_t *) struct_strides;
  static constexpr uint8_t *schema_binary_array = (uint8_t *) binary::array;
  static constexpr size_t schema_binary_size = binary::size;)");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "  static constexpr uint32_t schema_struct_count = ");
  output_decimal(ctx, structs.count);
  output_cstring(ctx, ";\n");

  output_cstring(ctx, "  static constexpr uint32_t min_read_scratch_memory_size = ");
  output_decimal(ctx, get_min_read_scratch_memory_size(schema_bytes, schema_definition));
  output_cstring(ctx, ";\n");

  output_cstring(ctx, "  static constexpr uint32_t compatibility_work_base = ");
  output_decimal(ctx, get_compatibility_work_base(schema_bytes));
  output_cstring(ctx, ";\n");

  output_cstring(ctx, "  static constexpr uint64_t schema_id = 0x");
  output_hexadecimal(ctx, schema_definition->schemaId);
  output_cstring(ctx, "ull;\n");

  output_cstring(ctx, "  static constexpr uint64_t content_hash = 0x");
  output_hexadecimal(ctx, get_content_hash(schema_bytes));
  output_cstring(ctx, "ull;\n");

  output_cstring(ctx, "};\n");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "// C++ trickery: _SchemaDescription::PerType.\n");

  for (UInt i = 0; i < structs.count; i++) {
    auto it = structs.pointer + i;
    output_cstring(ctx, "template<>\nstruct _SchemaDescription::PerType<");
    output_name(ctx, it->typeId);
    output_cstring(ctx, "> {\n  static constexpr uint64_t type_id = ");
    output_name(ctx, it->typeId);
    output_cstring(ctx, "_type_id;\n  static constexpr uint32_t index = ");
    output_name(ctx, it->typeId);
    output_cstring(ctx, "_struct_index;\n");
    output_cstring(ctx, "};\n\n");
  }

  output_cstring(ctx, "} // namespace ");
  output_name(ctx, schema_definition->schemaId);
  output_cstring(ctx, "\n\n");

  output_cstring(ctx, "namespace runtime {\n");
  output_cstring(ctx, "\n");
  output_cstring(ctx, "// C++ trickery: GetSchemaFromType.\n");
  for (UInt i = 0; i < structs.count; i++) {
    auto it = structs.pointer + i;
    output_cstring(ctx, "template<>\nstruct GetSchemaFromType<");
    output_name(ctx, schema_definition->schemaId);
    output_cstring(ctx, "::");
    output_name(ctx, it->typeId);
    output_cstring(ctx, "> {\n  using SchemaDescription = ");
    output_name(ctx, schema_definition->schemaId);
    output_cstring(ctx, "::_SchemaDescription;\n};\n\n");
  }

  output_cstring(ctx, "} // namespace runtime\n");
  output_cstring(ctx, "\n");

  // Note: ".h" and ".hpp" generated files currently embed the binary schema
  // separately, and if a program uses both for some reason, it will have to
  // link two copies. This is a bit weird, but unlikely and relatively harmless.

  output_cstring(ctx, "// Binary schema.\n");
  output_cstring(ctx, "#if defined(SVF_INCLUDE_BINARY_SCHEMA) || defined(SVF_IMPLEMENTATION)\n");
  output_cstring(ctx, "#ifndef SVF_");
  output_name(ctx, schema_definition->schemaId);
  output_cstring(ctx, "_BINARY_INCLUDED_HPP\n");

  output_cstring(ctx, "namespace ");
  output_name(ctx, schema_definition->schemaId);
  output_cstring(ctx, " {\n");
  output_cstring(ctx, "\n");

  output_cstring(ctx, "uint32_t const struct_strides[] = {\n");
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

  output_cstring(ctx, "uint8_t const array[] = {\n");
  output_raw_bytes(ctx, schema_bytes);
  output_cstring(ctx, "};\n");

  output_cstring(ctx, "\n");
  output_cstring(ctx, "} // namespace binary\n");
  output_cstring(ctx, "} // namespace ");
  output_name(ctx, schema_definition->schemaId);
  output_cstring(ctx, "\n");

  output_cstring(ctx, "#endif // SVF_");
  output_name(ctx, schema_definition->schemaId);
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
