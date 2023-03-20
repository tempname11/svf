# Main ideas

This is an unstructured brain-dump / idea-snapshot of this project as of March 08, 2023.

- your data is encoded as a single contigous buffer.
- (let's call it a "message")
- it should be less than 4GB (i.e. addressable with 32-bit values).
- messages have a schema.
- for debug/recreational/non-critical usage, the whole schema is part of the message.
- for serious production usage, only a hash of the schema is referenced.
- either way, it's possible to tell how to interpret the data, based on the message itself.

Schemas:

- there is a text-based human-friendly language to write schemas.
- but they are encoded into binary form for runtime.
- there is a meta-schema, i.e the description of how schemas are stored.

Runtime:

- the common case is that the data is written and read using the same schema.
- in this case, reading is almost a no-op, i.e. once you fill your buffer with data, and check that the schema hash matches, you are ready to use it.
- reading the data is essentially just using normal C structs, arrays, etc. with small caveats for "pointers" and unions.

Data evolution:

- sometimes, you write your data using one schema, but read it using a different one.
- backward compatibility: the old version of the program wrote the data to the disk. Then, the new version of the program is reading data from the disk.
- forward compatiibility: the newer server is providing more data. The older client does not know about it, but is able to read the data it did before.
- in the best case, data may change in a "binary" compatible way. no runtime cost is incurred for the reader.
- in a more general case, data may change in a "logically" compatible way. at runtime, the reader will first do some data manipulation, and then it will be ready for use.
- in the worst case, data will be incompatible, and this is an error.
- tools should provide easy understanding of which schemas are compatible with which, and whether it's "binary" or "logical" compatibility. 

Schema language:

- u8/u16/u32/u64 - unsigned
- i8/i16/i32/i64 - (signed) integer
- f32/f64 - floats
- struct
- tagged union (~= u8 tag + C-style union)
- array

Message format:

- header: magic, SUF version, flags (?)
- schema: hash, optional content, optional debug info (keywords, etc.)
- data follows.