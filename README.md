# Simple, Versatile Format

[![Build Status](https://github.com/tempname11/svf/actions/workflows/main.yml/badge.svg?branch=main)](https://github.com/tempname11/svf/actions/workflows/main.yml?query=branch:main) ![Alpha](https://img.shields.io/badge/beta-blue)

**SVF** is a format for structured data that is focused on being both machine-friendly, and human-friendly — in that order. Currently, it's in beta, meaning that all core features are finished, and no big changes are expected, only incremental improvements. However, 100% stability of the API, and of the schema format is not yet guaranteed.

More precisely, this project currently includes:
- A small text language to describe data schemas.
- A CLI tool for C/C++ code generation from said schemas.
- A C/C++ runtime, distributed as a single-file header library.

### Quick Start

FIXME

### Data Schemas

The format is structured, meaning that all data has an associated schema. Here is a silly example of a schema:

```rs
#name Hello

World: struct {
  population: U64; // 64-bit unsigned integer.
  gravitationalConstant: F32; // 32-bit floating-point.
  currentYear: I16; // 16-bit integer (signed).
  mechanics: Mechanics;
  name: String;
};

Mechanics: choice {
  classical;
  quantum;
  simulated: struct {
    parentWorld: World*;
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

### Code Examples

See:
- ["Hello" example schema](svf_tools/schema/Hello.txt), the same as above.
- ["Hello" written to file in C](svf_tools/src/test/hello_write.c).
- ["Hello" read from bytes in C++](svf_tools/src/test/hello_read.cpp).
- ["JSON" example schema](svf_tools/schema/JSON.txt), as a small exercise to encode JSON.
- ["JSON" written to file in C++](svf_tools/src/test/json_write.cpp).
- ["JSON" read from bytes in C](svf_tools/src/test/json_read.c).

### Alternatives

Here are some established formats you might want to use instead. They have their own drawbacks, which is why this project was started (more on this in [Rationale](svf_docs/Rationale.md)).

- Protocol Buffers: https://protobuf.dev/
- Cap'n Proto: https://capnproto.org/
- FlatBuffers: https://flatbuffers.dev/
- Apache Avro: https://avro.apache.org/
- CBOR: https://cbor.io/
- BSON: https://bsonspec.org/
- Text-based formats, like JSON, YAML, or XML.

### License

[MIT](./LICENSE.txt).
