#include <src/library.hpp>
#include "../core.hpp"
#include "common.hpp"
#include "output_common.hpp"

namespace core::output::c {

void output_struct_declaration(Ctx ctx, meta::StructDefinition *it) {
  output_cstring(ctx, "typedef struct SVF_");
  output_u8_array(ctx, ctx->in_schema->name);
  output_cstring(ctx, "_");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, " SVF_");
  output_u8_array(ctx, ctx->in_schema->name);
  output_cstring(ctx, "_");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, ";\n");
}

void output_choice_declaration(Ctx ctx, meta::ChoiceDefinition *it) {
  output_cstring(ctx, "typedef uint8_t SVF_");
  output_u8_array(ctx, ctx->in_schema->name);
  output_cstring(ctx, "_");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, "_enum;\n");

  output_cstring(ctx, "typedef union SVF_");
  output_u8_array(ctx, ctx->in_schema->name);
  output_cstring(ctx, "_");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, "_union SVF_");
  output_u8_array(ctx, ctx->in_schema->name);
  output_cstring(ctx, "_");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, "_union;\n");
}

void output_struct_index(Ctx ctx, svf::Array<U8> name, U32 index) {
  output_cstring(ctx, "#define SVF_");
  output_u8_array(ctx, ctx->in_schema->name);
  output_cstring(ctx, "_");
  output_u8_array(ctx, name);
  output_cstring(ctx, "_struct_index ");
  output_decimal(ctx, index);
  output_cstring(ctx, "\n");
}

void output_name_hash(Ctx ctx, svf::Array<U8> name, U64 hash) {
  output_cstring(ctx, "#define SVF_");
  output_u8_array(ctx, ctx->in_schema->name);
  output_cstring(ctx, "_");
  output_u8_array(ctx, name);
  output_cstring(ctx, "_name_hash 0x");
  output_hex(ctx, hash);
  output_cstring(ctx, "ull\n");
}

void output_concrete_type_name(
  Ctx ctx,
  meta::ConcreteType_enum in_enum,
  meta::ConcreteType_union *in_union
) {
  switch (in_enum) {
    case meta::ConcreteType_enum::defined_struct: {
      auto structs = to_range(ctx->in_bytes, ctx->in_schema->structs);
      output_cstring(ctx, "SVF_");
      output_u8_array(ctx, ctx->in_schema->name);
      output_cstring(ctx, "_");
      output_u8_array(ctx, structs.pointer[in_union->defined_struct.index].name);
      break;
    }
    case meta::ConcreteType_enum::defined_choice: {
      auto choices = to_range(ctx->in_bytes, ctx->in_schema->choices);
      output_cstring(ctx, "SVF_");
      output_u8_array(ctx, ctx->in_schema->name);
      output_cstring(ctx, "_");
      output_u8_array(ctx, choices.pointer[in_union->defined_choice.index].name);
      break;
    }
    case meta::ConcreteType_enum::u8: {
      output_cstring(ctx, "uint8_t");
      break;
    }
    case meta::ConcreteType_enum::u16: {
      output_cstring(ctx, "uint16_t");
      break;
    }
    case meta::ConcreteType_enum::u32: {
      output_cstring(ctx, "uint32_t");
      break;
    }
    case meta::ConcreteType_enum::u64: {
      output_cstring(ctx, "uint64_t");
      break;
    }
    case meta::ConcreteType_enum::i8: {
      output_cstring(ctx, "int8_t");
      break;
    }
    case meta::ConcreteType_enum::i16: {
      output_cstring(ctx, "int16_t");
      break;
    }
    case meta::ConcreteType_enum::i32: {
      output_cstring(ctx, "int32_t");
      break;
    }
    case meta::ConcreteType_enum::i64: {
      output_cstring(ctx, "int64_t");
      break;
    }
    case meta::ConcreteType_enum::f32: {
      output_cstring(ctx, "float");
      break;
    }
    case meta::ConcreteType_enum::f64: {
      output_cstring(ctx, "double");
      break;
    }
    default: {
      ASSERT(false);
    }
  }
}

void output_type(Ctx ctx, meta::Type_enum in_enum, meta::Type_union *in_union) {
  switch(in_enum) {
    case meta::Type_enum::concrete: {
      output_concrete_type_name(
        ctx,
        in_union->concrete.type_enum,
        &in_union->concrete.type_union
      );
      break;
    }
    case meta::Type_enum::pointer: {
      output_cstring(ctx, "SVFRT_Pointer /*");
      output_concrete_type_name(
        ctx,
        in_union->concrete.type_enum,
        &in_union->concrete.type_union
      );
      output_cstring(ctx, "*/");
      break;
    }
    case meta::Type_enum::flexible_array: {
      output_cstring(ctx, "SVFRT_Array /*");
      output_concrete_type_name(
        ctx,
        in_union->concrete.type_enum,
        &in_union->concrete.type_union
      );
      output_cstring(ctx, "*/");
      break;
    }
    default: {
      ASSERT(false);
    }
  }
}

Bool output_struct(Ctx ctx, meta::StructDefinition *it) {
  auto structs = to_range(ctx->in_bytes, ctx->in_schema->structs);
  auto choices = to_range(ctx->in_bytes, ctx->in_schema->choices);

  output_cstring(ctx, "struct SVF_");
  output_u8_array(ctx, ctx->in_schema->name);
  output_cstring(ctx, "_");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, " {\n");

  UInt size_sum = 0;
  auto fields = to_range(ctx->in_bytes, it->fields);

  for (UInt i = 0; i < fields.count; i++) {
    auto field = fields.pointer + i;

    // We don't support custom struct layouts yet.
    // Also, will break on @proper-alignment.
    if (field->offset != size_sum) {
      return false;
    }

    auto plurality = get_plurality(
      structs,
      choices,
      field->type_enum,
      &field->type_union
    );
    size_sum += plurality.size;

    switch (plurality.plurality) {
      case TypePlurality::zero: {
        continue;
      }
      case TypePlurality::one: {
        output_cstring(ctx, "  ");
        output_type(ctx, field->type_enum, &field->type_union);
        output_cstring(ctx, " ");
        output_u8_array(ctx, field->name);
        output_cstring(ctx, ";\n");
        break;
      }
      case TypePlurality::enum_and_union: {
        if (field->type_enum != meta::Type_enum::concrete) {
          return false;
        }

        output_cstring(ctx, "  ");
        output_concrete_type_name(
          ctx,
          field->type_union.concrete.type_enum,
          &field->type_union.concrete.type_union
        );
        output_cstring(ctx, "_enum ");
        output_u8_array(ctx, field->name);
        output_cstring(ctx, "_enum;\n  ");
        output_concrete_type_name(
          ctx,
          field->type_union.concrete.type_enum,
          &field->type_union.concrete.type_union
        );
        output_cstring(ctx, "_union ");
        output_u8_array(ctx, field->name);
        output_cstring(ctx, "_union;\n");
        break;
      }
      default: {
        ASSERT(false);
      }
    }
  }

  // Will break on @proper-alignment.
  if (size_sum != it->size) {
    return false;
  }

  output_cstring(ctx, "};\n\n");
  return true;
}

Bool output_choice(Ctx ctx, meta::ChoiceDefinition *it) {
  auto structs = to_range(ctx->in_bytes, ctx->in_schema->structs);
  auto choices = to_range(ctx->in_bytes, ctx->in_schema->choices);

  auto options = to_range(ctx->in_bytes, it->options);
  UInt size_max = 0;

  for (UInt i = 0; i < options.count; i++) {
    auto option = options.pointer + i;
    output_cstring(ctx, "#define SVF_");
    output_u8_array(ctx, ctx->in_schema->name);
    output_cstring(ctx, "_");
    output_u8_array(ctx, it->name);
    output_cstring(ctx, "_");
    output_u8_array(ctx, option->name);
    output_cstring(ctx, " ");
    output_decimal(ctx, safe_int_cast<U64>(option->index));
    output_cstring(ctx, "\n");
  }

  output_cstring(ctx, "\nunion SVF_");
  output_u8_array(ctx, ctx->in_schema->name);
  output_cstring(ctx, "_");
  output_u8_array(ctx, it->name);
  output_cstring(ctx, "_union {\n");
  for (UInt i = 0; i < options.count; i++) {
    auto option = options.pointer + i;

    auto plurality = get_plurality(
      structs,
      choices,
      option->type_enum,
      &option->type_union
    );
    size_max = maxi(size_max, plurality.size);

    if (plurality.plurality == TypePlurality::zero) {
      continue;
    }
    if (plurality.plurality != TypePlurality::one) {
      return false;
    }

    output_cstring(ctx, "  ");
    output_type(ctx, option->type_enum, &option->type_union);
    output_cstring(ctx, " ");
    output_u8_array(ctx, option->name);
    output_cstring(ctx, ";\n");

  }

  if (size_max != it->payload_size) {
    return false;
  }

  output_cstring(ctx, "};\n\n");
  return true;
}

Bytes as_code(
  vm::LinearArena *arena,
  Bytes schema_bytes,
  validation::Result *validation_result
) {
  // This will break when proper alignment is done. @proper-alignment
  auto in_schema = (meta::Schema *) (
    schema_bytes.pointer +
    schema_bytes.count -
    sizeof(meta::Schema)
  );

  auto start = vm::realign(arena);
  OutputContext context_value = {
    .dedicated_arena = arena,
    .in_schema = in_schema,
    .in_bytes = schema_bytes,
  };

  auto ctx = &context_value;
  auto structs = to_range(schema_bytes, in_schema->structs);
  auto choices = to_range(schema_bytes, in_schema->choices);

  output_cstring(ctx, "// AUTOGENERATED by svfc.\n");
  output_cstring(ctx, "#ifndef SVF_");
  output_u8_array(ctx, in_schema->name);
  output_cstring(ctx, "_H\n#define SVF_");
  output_u8_array(ctx, in_schema->name);
  output_cstring(ctx, "_H\n");
  output_cstring(ctx, R"(
#ifdef __cplusplus
#include <cstdint>
#include <cstddef>

extern "C" {
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifndef SVF_COMMON_C_TYPES_INCLUDED
#define SVF_COMMON_C_TYPES_INCLUDED
#pragma pack(push, 1)

typedef struct SVFRT_Pointer {
  uint32_t data_offset;
} SVFRT_Pointer;

typedef struct SVFRT_Array {
  uint32_t data_offset;
  uint32_t count;
} SVFRT_Array;

#pragma pack(pop)
#endif // SVF_COMMON_C_TYPES_INCLUDED

#pragma pack(push, 1)

)");

  output_cstring(ctx, "#define SVF_");
  output_u8_array(ctx, in_schema->name);
  output_cstring(ctx, "_min_read_scratch_memory_size ");
  output_decimal(ctx, get_min_read_scratch_memory_size(in_schema));
  output_cstring(ctx, "\n");

  output_cstring(ctx, "#define SVF_");
  output_u8_array(ctx, in_schema->name);
  output_cstring(ctx, "_binary_size ");
  output_decimal(ctx, schema_bytes.count);
  output_cstring(ctx, "\nextern uint8_t const SVF_");
  output_u8_array(ctx, in_schema->name);
  output_cstring(ctx, "_binary_array[];\n\n#ifdef SVF_INCLUDE_BINARY_SCHEMA\nuint8_t const SVF_");
  output_u8_array(ctx, in_schema->name);
  output_cstring(ctx, "_binary_array[] = {\n");
  output_raw_bytes(ctx, schema_bytes);
  output_cstring(ctx, "};\n#endif // SVF_INCLUDE_BINARY_SCHEMA\n\n// Forward declarations.\n");
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
    output_name_hash(ctx, it->name, it->name_hash);
  }
  for (UInt i = 0; i < choices.count; i++) {
    auto it = choices.pointer + i;
    output_name_hash(ctx, it->name, it->name_hash);
  }

  output_cstring(ctx, "\n// Full declarations.\n");

  for (UInt i = 0; i < validation_result->ordering.count; i++) {
    auto item = validation_result->ordering.pointer + i;

    if (item->type == meta::ConcreteType_enum::defined_struct) {
      if (!output_struct(ctx, structs.pointer + item->index)) {
        return {};
      }
    } else if (item->type == meta::ConcreteType_enum::defined_choice) {
      if (!output_choice(ctx, choices.pointer + item->index)) {
        return {};
      }
    } else {
      return {};
    }
  }

  output_cstring(ctx, "#pragma pack(pop)\n#ifdef __cplusplus\n} // extern \"C\"\n #endif\n\n#endif // SVF_");
  output_u8_array(ctx, in_schema->name);
  output_cstring(ctx, "_H\n");

  auto end = arena->reserved_range.pointer + arena->waterline;
  return {
    .pointer = (Byte *) start,
    .count = offset_between<U64>(start, end),
  };
}

} // namespace output::c
