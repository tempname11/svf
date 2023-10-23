#undef NDEBUG
#include <assert.h>
#include <string.h>

#include <generated/h/Hello.h>
#include <src/svf_runtime.h>
#include <src/svf_stdio.h>

void example_write(FILE *file) {
  SVFRT_WriteContext ctx;
  SVFRT_WRITE_START(SVF_Hello, SVF_Hello_World, &ctx, SVFRT_fwrite, file);

  SVF_Hello_World world = {0};
  world.population = 8000000000;
  world.gravitationalConstant = 6.674e-11f;
  world.currentYear = 2023;
  world.mechanics_enum = SVF_Hello_Mechanics_quantum;

  char const name[] = "The Universe"; // TODO: null-termination is not what we want.
  world.name.utf8 = SVFRT_WRITE_FIXED_SIZE_ARRAY(&ctx, name);

  SVFRT_WRITE_FINISH(&ctx, &world);
  assert(ctx.finished);
}
