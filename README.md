# Simple Universal Format

## Problem statement

Let us say that your program is written in C or C++ for a typical modern machine, that is, 64-bit little-endian. You want to serialize and deserialize moderate amounts of data, either:

- to/from a file.
- over the network.

Your concerns are:

- no complex build-time dependencies.
- no complex run-time dependencies.
- code to be readable and debuggable.
- code to be reasonably efficient.
- you want to be able to evolve your data over time.
- you are not constrained by compatibility reasons.

## Available solutions

- ~~JSON~~ (just kidding)
- Protocol buffers: https://protobuf.dev/
- Cap'n Proto: https://capnproto.org/
- Flatbuffers: https://google.github.io/flatbuffers/

I will not argue against any of these, but in my opinion, these solutions are more than many problems truly require. Perhaps we can do more with less?

## Outline

Let's now clarify what this project aims to be:

- a small schema language specification,
- a code generation tool for C and C++
- and a debugging tool to inspect data.
- documentation on how everything works.

If you find this interesting, jump into specific topics.