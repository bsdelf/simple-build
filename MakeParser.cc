#include "MakeParser.h"

ArgumentParser MakeParser() {
  return ArgumentParser()
    .On("clean", "clean files", ArgumentParser::Set("0", "1"))
    .On("jobs", "set number of jobs", ArgumentParser::Set("0", "0"))
    .On("target", "set target name", ArgumentParser::Set("a.out", "a.out"))
    .On("workdir", "set working directory", ArgumentParser::Set(".", "."))
    .On("verbose", "set verbose level", ArgumentParser::Set("0", "1"))
    .Split()
    .On("as", "set assembler", ArgumentParser::Set("as", "as"))
    .On("asflags", "add assembler flags", ArgumentParser::Join("", {}))
    .On("cc", "set c compiler", ArgumentParser::Set("cc", "cc"))
    .On("cflags", "add c compiler flags", ArgumentParser::Join("", {}))
    .On("cxx", "set c++ compiler", ArgumentParser::Set("c++", "c++"))
    .On("cxxflags", "add c++ compiler flags", ArgumentParser::Join("", {}))
    .On("ld", "set linker", ArgumentParser::Set("cc", "cc"))
    .On("ldflags", "add linker flags", ArgumentParser::Join("", {}))
    .On("prefix", "add search directories",
        ArgumentParser::JoinTo(
          "cflags", {}, {},
          [](auto& input) { input = "-I" + input + "/include"; }),
        ArgumentParser::JoinTo(
          "cxxflags", {}, {},
          [](auto& input) { input = "-I" + input + "/include"; }),
        ArgumentParser::JoinTo(
          "ldflags", {}, {},
          [](auto& input) { input = "-L" + input + "/lib"; }))
    .Split()
    .On("wol", "without link", ArgumentParser::Set("0", "1"))
    .On("thread", "use pthreads",
        ArgumentParser::JoinTo("cflags", {}, "-pthread"),
        ArgumentParser::JoinTo("cxxflags", {}, "-pthread")
#ifndef __APPLE__
          ,
        ArgumentParser::JoinTo("ldflags", {}, "-pthread")
#endif
          )
    .On("optimize", "set optimize level",
        ArgumentParser::JoinTo("cflags", {}, "2", [](auto& input) { input = "-O" + input; }),
        ArgumentParser::JoinTo("cxxflags", {}, "2", [](auto& input) { input = "-O" + input; }))
    .On("debug", "enable -g",
        ArgumentParser::JoinTo("cflags", {}, "-g"),
        ArgumentParser::JoinTo("cxxflags", {}, "-g"))
    .On("release", "enable -DNDEBUG",
        ArgumentParser::JoinTo("cflags", {}, "-DNDEBUG"),
        ArgumentParser::JoinTo("cxxflags", {}, "-DNDEBUG"))
    .On("strict", "enable -Wall -Wextra -Werror",
        ArgumentParser::JoinTo("cflags", {}, "-Wall -Wextra -Werror"),
        ArgumentParser::JoinTo("cxxflags", {}, "-Wall -Wextra -Werror"))
    .On("shared", "enable -fPIC -shared",
        ArgumentParser::JoinTo("cflags", {}, "-fPIC"),
        ArgumentParser::JoinTo("cxxflags", {}, "-fPIC"),
#ifdef __APPLE__
        ArgumentParser::JoinTo("ldflags", {}, "-dynamiclib"))
#else
        ArgumentParser::JoinTo("ldflags", {}, "-shared"))
#endif
    .On("lto", "enable -flto", ArgumentParser::JoinTo("ldflags", {}, "-flto"))
    .On("c89", "enable -std=c89", ArgumentParser::JoinTo("cflags", {}, "-std=c89"))
    .On("c99", "enable -std=c99", ArgumentParser::JoinTo("cflags", {}, "-std=c99"))
    .On("c11", "enable -std=c11", ArgumentParser::JoinTo("cflags", {}, "-std=c11"))
    .On("c18", "enable -std=c18", ArgumentParser::JoinTo("cflags", {}, "-std=c18"))
    .On("c++11", "enable -std=c++11", ArgumentParser::JoinTo("cxxflags", {}, "-std=c++11"))
    .On("c++14", "enable -std=c++14", ArgumentParser::JoinTo("cxxflags", {}, "-std=c++14"))
    .On("c++17", "enable -std=c++17", ArgumentParser::JoinTo("cxxflags", {}, "-std=c++17"))
    .On("c++20", "enable -std=c++20", ArgumentParser::JoinTo("cxxflags", {}, "-std=c++20"))
    .Split()
    .On("help", "show help", ArgumentParser::Set("0", "1"));
}