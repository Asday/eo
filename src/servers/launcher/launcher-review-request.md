# Description

For fun, and to learn some C++, I've decided to reimplement the entirety of EVE online as I remember it.  (I'm aware this is an insane and likely unreachable goal, that's intentional).

I've started on the server side of things as I'm a little more comfortable with that than graphics programming, and settled on a cluster design, with one main `cluster` application handling startup and shutdown of the universe, one `launcher` application per server, one `station` server per space station, one `grid` server per simulated space instance, and so forth.

Below is the _somewhat_ finished `launcher`.  It starts up, asks the database for its own IP address, and updates the database every second-ish as a sort of health check.  At the same time, it opens a UDP socket, and listens for messages.

The messages start with their version, then the message type, then the body of that message (differing based on type), and ending with a sentinel value of `0x6C`.

Two types of message are valid, one to launch instances, and one to shut down instances.  I've not written any of the other servers yet, so how to shut them down is as yet undiscovered, and the shutdown message in `launcher` is left unimplemented.

`launcher` caps itself off by listening for `SIGTERM` or `SIGINT`, and gracefully shutting down when such a signal arrives.

I've avoided using exceptions, except in constructors, where they're used to gracefully tear down an object from within an initialiser list.  This appears to be required for RAII, but I really don't like exceptions - I don't like their ability to bubble up and kill the server.  There's little worse than playing a video game and having it _hard_ crash to the desktop because some exception deep in the bowels of a particle system that the player doesn't really care about that much sneaks by.

To that end, all constructors that throw are private, and the objects are constructed with `std::expected<T, E> T::tryCreate()`.

# Things I plan to do at some point and maybe don't need to be told about

* There's a lambda being used to `cout` a `variant` of exceptions in `main()` which may be able to be replaced with `cout`ing the `variant` directly now I implemented `operator<<` for `variant`s.
* There's a bug when it comes to not clearing the old packet buffer, then receiving an incomplete message.  I plan to fix this by implementing `struct Packet` that will provide `std::expected<ValidatedPacket, std::string> getValidatedPacket()`, where the `ValidatedPacket` is some view onto the underlying buffer with the transport level anti corruption checks stripped out.
* I will be splitting this into different files.  Just wanted to get it running and learn stuff along the way, first.
* The heartbeat will record compute/memory pressure stats in the database alongside the `launcher`'s IP in the future, such that `cluster` can pick an appropriate server on which to launch new instances.  Right now there's not a great deal of reason to update the heartbeat in the database each second as I do.
* I've shyed away from external libraries (apart from `libpq`) mostly to learn how things work "under the hood" myself, but also in a small effort to avoid "bloat".  I'm aware Boost provides things like a UUID class and networking stuff, but I'm under the impression Boost is mega-huge, and I'd like to not use stuff like that, at least for now, unless there's an insanely compelling reason to.  (For the client, I'm sure I'll use something like raylib, so external dependencies will come).
* String interpolation for SQL instead of paramater passing - I know.  Trust me I do.  I've gone with this way _for now_ because I want to visit [sqlc](https://sqlc.dev) for repository construction, which is going to require some work, at which point I'll deal with parameter passing, prepared statements, and probably more.

# Specific things that could be better that I'd like to know about.

* I haven't used RAII for saving and restoring `os.flags()` in `operator<<(std::ostream&, const UUID&)`.  It seems like it might be overkill, but it also seems like exactly the sort of place I should be using RAII.  (I'd use a context manager in Python).
* `UUID` instantiation feels weird.  Having to directly name every offset from `[0]` to `[15]`, and providing a way to unsafely create a `UUID` from `const char*`...  I don't know, I just don't like it.
* In a couple places, I'm turning off `-Wmissing-field-initializers` when constructing `sockaddr_in`, which doesn't seem right.  I'm pretty sure I'm setting every field I need to in the structs, but the compiler still whinges.
* I would love to know how to write `template <typenameT, typename... Ts> operator<<(std::ostream& const std::variant<T, Ts...>&)` with `auto` and concepts instead of a template.
* I don't like being able to use `static_cast<ScopedEnum>(...)` to make nonsensical values, and thus being forced to handle that in functions accepting a scoped enum.
* Can I usefully use constexpr in more places?
* When launching the child processes, their stdout appears in the same terminal as the launcher.  I feel like I don't want that, but then I also don't know where else I'd like it to go.  I do plan to write up a logger that will have different logging levels enabled based on environment variables, log to files and the terminal, prefix with log level and executable name, etc., but that's in the future.
* Defining move-only classes seems really verbose and boilerplatey.  Is there anything I can do to avoid that?  (Preferably not using the preprocessor).

# The main questions

Finally, I'd like to know if there are any particularly unsustainable practices I'm engaging in not mentioned above, any STL things I'm not using that I should be, or using more than I should be, and anything else that stands out as completely wrong to someone with a professional history in C++.

# Compilation

You'll need `libpq` available - `pg-config --includedir` and `pg-config --libdir` should work.

```sh
g++ \
	-I/usr/include -std=c++23 -O3 -Wall -Wextra -Werror -Wsign-conversion -pedantic-errors -g \
	-L/usr/lib -lpq  \
	launcher.cpp \
	-o launcher
```

# Execution

Only tested on Linux.  As this is a server, I don't plan to run it on anything but.

DDL:

```sql
CREATE TABLE public.launcher (
	id uuid DEFAULT gen_random_uuid() NOT NULL,
	ip inet NOT NULL,
	port int4 NOT NULL,
	heartbeat timestamp NOT NULL,
	CONSTRAINT launcher_pk PRIMARY KEY (id)
);
```

Set environment variables to point to your PostgreSQL database:

```sh
export PGHOST=127.0.0.1
export PGPORT=5432
export PGUSER=postgres
export PGDATABASE=eo
```

# Source

[`main.cpp`](./main.cpp)
