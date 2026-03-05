# FO4 Plugins

This repository is a monorepo that houses multiple Fallout 4 mod projects and shared libraries used across them.

The purpose is to keep the build infrastructure and related / shared code together for simplified dependency management.

## CommonLibF4

This repo uses [my fork](https://github.com/kamicane/Commonlibf4) of [LucaDotGit's CommonLibF4 NG](https://github.com/LucaDotGit/Commonlibf4), which adds support for AE.

Builds are multi-version. The plugin working in a specific version depends on the ID availability. I am not considering NG at all.

## Layout

- `Projects/PROJECT_ID/`: separate projects
- `extern/`: CommonLibF4 libraries
- `include/`: shared headers used across projects
- `Interface/`: shared interface files, using decompiled actionscript sources from [F4CF/Interface](https://github.com/F4CF/Interface) for building

Building is done with CMake presets. It switches what ./src/main.cpp pulls in. Each subproject provides a project.hpp file with a common export interface so that main.cpp can read it. Projects do not provide cpp files, only headers.

vcpkg is used for external package management (see `vcpkg.json`)

## Code server

I use vscode with the clangd extension. This repo assumes the correct `compile_commands.json` for the desired project is copied in the root (the cmake vscode extension can do that). Clangd will require a restart after every preset switch.

clangd works mostly fine, but might crash if you open one too many CommonLibF4 headers.

cpptools is just too slow to be viable, especially with this setup.

## Building

Building is done with msvc. This means vscode will have to be opened with the correct environment.
