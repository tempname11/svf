# Simple, Versatile Format

[![Build Status](https://github.com/tempname11/svf/actions/workflows/main.yml/badge.svg?branch=main)](https://github.com/tempname11/svf/actions/workflows/main.yml) ![Alpha](https://img.shields.io/badge/alpha-orange)

**SVF** is a format for structured data that is focused on being both machine-friendly, and human-friendly — in that order. Currently, it's in alpha, meaning that while most of the core functionality is working, there is still a lot to be done (see [Roadmap](#roadmap)).

More precisely, this project currently includes:
- A small text language to describe data schemas.
- A CLI tool to work with the schemas.
- C and C++ [runtime libraries](./svf_runtime/src) to actually work with data.

### Machine-friendly

One of the design goals for the format is simplicity, which includes "how many things the machine has to do, to read or write the data". So, the format is binary, and aside from [versioning hurdles](#data-evolution), reading it essentially involves repeatedly viewing regions of memory as known structures (i.e. C/C++ structs, with caveats). Writing it is similarly straightforward, with typical scenarios mostly copying structures to output buffers.

### Human-friendly

Text-based formats like JSON seem to be everywhere these days, so are they better? For people, it's not easy to read binary blobs, let alone modify them by hand. The assumption here is that a large part of the problem is simply solved by good tooling, and for many domains, the remaining drawbacks are outweighted by the benefits.

A key part of this approach is developing a good GUI viewer/editor for the format, which is on the [roadmap](#roadmap).

### Data Schemas

The format is structured, meaning that all data has an associated schema. Here is a silly example of a schema:

```rs
#name Hello

World: struct {
  population: U64; // 64-bit unsigned integer.
  gravitational_constant: F32; // 32-bit floating-point.
  current_year: I16; // 16-bit integer (signed).
  mechanics: Mechanics;
  name: String;
};

Mechanics: choice {
  classical;
  quantum;
  simulated: struct {
    parent_world: World*;
  };
};

String: struct {
  utf8: U8[];
};
```

Note, that the concept of a "string" is not built-in, but can be easily expressed in a schema. In this instance, UTF-8 is the chosen encoding, and expressed how you would expect — as a sequence of 8-bit characters.

### Data Evolution

The above schema may resemble `struct` declarations in C/C++. In fact, the CLI code generator will output structures very much like what you would expect. What's the benefit over writing these C/C++ declarations by hand? Well, programs change over time, with their data requirements also changing. There are two typical scenarios:

- Data is written into a file by a program. Later, a newer version of that program, reads it back. This requires **backward compatibility**, if the schema was modified.
- Data is being sent over the network, say, from server to client. The server is updated. The client is now older than the server, but still needs to process the message. This requires **forward compatibility**, if the server schema was modified.

Both of these scenarios are covered at runtime, by:
- Checking the schema compatibility before reading the message. This is currently a required step, but could potentially be skipped, if the schemas are the same, or the compatibility has already been checked before.
- Converting the data to the new schema, if the schemas are not "binary-compatible", and the calling code allows the conversion to happen.

### Usage

See:
- ["Hello" example schema](svf_tools/schema/Hello.txt), the same as above.
- ["Hello" written to file in C](svf_tools/src/test/hello_write.c).
- ["Hello" read from bytes in C++](svf_tools/src/test/hello_read.cpp).
- ["JSON" example schema](svf_tools/schema/JSON.txt), as a small exercise to encode JSON.
- ["JSON" written to file in C++](svf_tools/src/test/json_write.cpp).
- ["JSON" read from bytes in C](svf_tools/src/test/json_read.c).

### Roadmap

There is a number of things left to do before **beta** status would be appropriate, which would signify that the project is tentatively ready for "serious" usage. This list is not comprehensive, and not in any priority order:

- Adversarial inputs need to be handled well in the runtime libraries. Since the format is binary, and the code is written in C, there's a lot of potential for exploits. There are some guardrails in place already, and nothing prevents safety in theory, but there was no big, focused effort in this direction yet.
- Better error handling, with readable error messages where applicable (e.g. `svfc` output). Currently, a lot of code simply sets an error flag, it gets checked later, and that's about it.
- More extensive testing. Some basic tests are in-place, but something more serious needs to be done to catch corner cases. Perhaps, a fuzzy test case generator.
- Documentation and examples. Currently, there are a few tests that are doubling as examples, but not much more than that.
- Support of various platforms, and clear indication as to which are supported. This includes OS, CPU architectures & endianness, compilers, and language standards.
- Schema-less messages, and generally more focus on dealing with schemas at runtime. Right now, the full schema is always included in the message, which is simple, and easy to debug. But e.g. in network scenarios, where the data might be much smaller than the schema, and transferred often, excluding it would be critical. This means that the user's code would need some other way to get the schema. Also, checking schema compatibility is not free, and skipping it should be possible in some cases.
- Once the data format is stabilized, some kind of unambiguous specification, whether formal, or informal, needs to be written down. This is also true for the schema text language, although that is less important.
- A GUI tool to work with data directly, which is essential for debugging, and, say, using the format for any configuration files. It needs to be easy to use, and runnable on developer machines.

### Alternatives

Here are some established formats you might want to use instead. They have their own drawbacks, which is why this project was started.

- Protocol Buffers: https://protobuf.dev/
- Cap'n Proto: https://capnproto.org/
- FlatBuffers: https://flatbuffers.dev/
- Text-based formats, like JSON, YAML, or XML.

### License

[MIT](./LICENSE.txt).
