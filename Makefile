OBJECTS = Async.o \
          Error.o \
          IOPoll.o \
          Log.o \
          Main.o \
          MemoryPool.o \
          RunFiber.o \
          Runtime.o \
          Scheduler.o \
          Timer.o

override CPPFLAGS += -iquote Include -MMD -MT $@ -MF Build/$*.d
override CXXFLAGS += -std=c++11 -Wall -Wextra -Wno-sign-compare -Wno-invalid-offsetof -Werror

all: Build/Library.a

Build/Library.a: $(addprefix Build/, $(OBJECTS))
	$(AR) rc $@ $^

ifneq ($(MAKECMDGOALS), clean)
-include $(patsubst %.o, Build/%.d, $(OBJECTS))
endif

Build/%.o: Source/%.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f Build/*

install: all
	cp -T Build/Library.a /usr/local/lib/libtara.a
	cp -r -T Include /usr/local/include/Tara

uninstall:
	rm -f /usr/local/lib/libtara.a
	rm -r -f /usr/local/include/Tara
