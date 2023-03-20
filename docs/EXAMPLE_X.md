# Examples

For now, this is fictional.

Schema:
```c
#name my_schema

request: struct {
  ID: u64,
  Header: request_header,
  Content: u8[],
}

request_header: struct {
  Foo: i32,
  Bar: union {
    First: void,
    Second: f32,
    Third: something_else[],
}

something_else: struct {
  ID: u64,
}
```

=>

C++
```cpp
namespace suf {
  template<typename T>
  struct array<T> {
    uint32_t count;
    uint32_t relptr;
  };

  template<typename T>
  struct tagged_union<T> {
    uint8_t tag;
    T content;
  };
}

namespace my_schema {
  struct request;
  struct request_header;
  struct something_else;

  struct request {
    uint64_t ID;
    request_header Header;
    suf::array<uint8_t> Content;
  };

  struct request_header {
    int32_t Foo;
    enum class enum_for_Bar: uint8_t {
      First,
      Second,
      Third,
    };
    union union_for_Bar {
      // void First;
        float Second;
        suf::array<something_else> Third;
    };
    suf::tagged_union<enum_for_Bar, union_for_Bar> Bar;
  };

}
```