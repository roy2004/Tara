#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#
#include "Runtime.hxx"

int Main(int argc, char **argv)
{
  Tara::Call([] () {
    Tara::Yield();
    try {
      throw 1;
    } catch (int x) {
      printf("1: %d\n", x);
    }
  });
  Tara::Call([] () {
    try {
      Tara::Yield();
      throw 2;
    } catch (int x) {
      printf("2: %d\n", x);
    }
  });
}
