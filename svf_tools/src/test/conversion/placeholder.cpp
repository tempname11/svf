#define SVF_INCLUDE_BINARY_SCHEMA
#include <src/svf_runtime.hpp>
#include "common.hpp"

int main(int /*argc*/, char */*argv*/[]) {
  // TODO: this is simply a placeholder for future tests. Some conversion error
  // codes are likely not even reachable from `SVFRT_read_message`, because the
  // compatibility check already catches the errors.

  // Here are the unreachable codes at the moment of writing this:
  //
  // - SVFRT_code_conversion__schema_type_tag_mismatch
  // - SVFRT_code_conversion__schema_concrete_type_tag_mismatch
  // - SVFRT_code_conversion__bad_type
  // - SVFRT_code_conversion__bad_schema_structs
  // - SVFRT_code_conversion__bad_schema_choices
  // - SVFRT_code_conversion__bad_schema_fields
  // - SVFRT_code_conversion__bad_schema_options
  // - SVFRT_code_conversion__bad_schema_struct_index
  // - SVFRT_code_conversion__bad_schema_field_index
  // - SVFRT_code_conversion__bad_schema_choice_index
  // - SVFRT_code_conversion__bad_schema_option_index
  // - SVFRT_code_conversion__bad_schema_type_tag
  //
  // If conversion is exposed as public API, then one might pass in invalid
  // schema, and these will be reachable, at which point tests will need to be
  // added.
  return 0;
}
