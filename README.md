<h1 align="center">sling</h1>

<p align="center">
    <a href="https://opensource.org/licenses/MIT">
        <img alt="MIT license" src="https://img.shields.io/github/license/adrg/sling" />
    </a>
    <a href="https://github.com/adrg/sling/issues">
        <img alt="GitHub issues" src="https://img.shields.io/github/issues/adrg/sling" />
    </a>
    <a href="https://ko-fi.com/T6T72WATK">
        <img alt="Buy me a coffee" src="https://img.shields.io/static/v1.svg?label=%20&message=Buy%20me%20a%20coffee&color=579fbf&logo=buy%20me%20a%20coffee&logoColor=white" />
    </a>
</p>

sling is a lightweight C++ implementation of signals and slots. A slot is
essentially a callback wrapper which can be created from different kinds of
callable entities. Slots can be connected to a signal, and they get notified
when the signal is emitted.

## Installation

Being a header-only library, you can just include it in your project. Requires
compiler support for C++11 or later.

## Usage

#### Creating signals
```cpp
sl::Signal<> sig0;            // signal without parameters.
sl::Signal<std::string> sig1; // signal with one parameter (std::string).
sl::Signal<double, int> sig2; // signal with two parameters (double and int).
```

#### Connecting slots
```cpp
sl::Signal<int> sig;
```

Connect slots created from lambda expressions.
```cpp
sig.connect(sl::Slot<int>([](int x) {}));
```

Connect slots created from regular functions.
```cpp
void foo(int x) {}
sig.connect(sl::Slot<int>(foo));
```

Connect slots created from object member functions.
```cpp
class Foo
{
public:
    void bar(int x) {}
    void baz(int x) const {}
    virtual void qux(int x) {}
    static void corge(int x) {}
};

Foo foo;
sig.connect(sl::Slot<int>(&foo, &Foo::bar));
sig.connect(sl::Slot<int>(&foo, &Foo::baz));
sig.connect(sl::Slot<int>(&foo, &Foo::qux));
sig.connect(sl::Slot<int>(Foo::corge));
```

Connect slots created from object instances implementing operator ().
```cpp
struct Bar
{
    void operator () (int x) const {}
};

sig.connect(sl::Slot<int>(Bar()));
```

#### Disconnecting slots
```cpp
void foo(int x) {}
sl::Signal<int> sig;
```

Disconnect using the slot interface.
```cpp
sl::Slot<int> slot(foo);
sig.connect(slot);
slot.disconnect();
```

Disconnect by passing a reference or a pointer to the slot.
```cpp
sl::Slot<int> slot(foo);
sig.connect(slot);
sig.disconnect(slot); // or sig.disconnect(&slot);
```

Disconnect by passing the slot key returned on signal connection.
```cpp
sl::SlotKey key = sig.connect(sl::Slot<int>(foo));
sig.disconnect(key);
```

Disconnect all slots.
```cpp
sig.clear();
```

All slots are automatically disconnected when the signal they are connected to
goes out of scope. Similarly, slots are automatically disconnected from signals
when they go out of scope.

Slots are suitable as public class members, as they are automatically disconnected
when the class instance they belong to goes out of scope. In addition, this
allows private member functions to be specified as slot callbacks.
```cpp
class Baz
{
private:
    void bar(int x) {}
public:
    sl::Slot<int> onBar{this, &Baz::bar};
};

Baz b;
sig.connect(b.onBar);
```

#### Emitting signals
```cpp
sl::Signal<int> sig;
sig(1); // or sig.emit(1);
```

#### Copying and moving

Signal copying is disabled as I have not found an intuitive behaviour for
this operation. Moving signals is allowed and it transfers all connected slots
to the new signal. The moved signal is left in a valid state, as if newly constructed.

Slot copying is permitted and the result of the operation is a slot having a
copy of the source slot callback. However, the new slot is not connected to the
signal the original slot is connected to, if any. When moving slots, the new
slot basically replaces the old one. If the moved slot is connected to a signal,
it is disconnected first, and the new one is connected to that signal.
The moved slot is left in a valid state, having no callback. A new callback
can be assigned using the `setCallback` method.

#### Thread safety

The library does not guarantee thread safety in its current state. I am
considering making it thread safe in the future.

## Contributing

Contributions in the form of pull requests, issues or just general feedback,
are always welcome.  
See [CONTRIBUTING.MD](CONTRIBUTING.md).

## License
Copyright (c) 2019 Adrian-George Bostan.

This project is licensed under the [MIT license](http://opensource.org/licenses/MIT).
See [LICENSE](LICENSE) for more details.
