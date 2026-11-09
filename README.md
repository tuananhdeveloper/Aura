# Aura

Aura is a C++ game framework built on [raylib](https://www.raylib.com/), with a native desktop generator built with [raygui](https://github.com/raysan5/raygui) for creating starter projects. It targets Android, iOS, desktop, and the web.

The starter includes a sample game and shared framework components for screens and lifecycle, input, viewports, assets, preferences, UI, tilemaps, tweens, and particles.

## Create a project

Download the package for your system from [GitHub Releases](https://github.com/tuananhdeveloper/Aura/releases): universal macOS (`.dmg`, for Apple Silicon and Intel), Linux x64 (`.tar.gz`), or Windows x64 (`.zip`). On macOS, open the disk image and move **Aura Generator.app** to Applications. The app bundle includes its starter template and resources. These macOS builds are unsigned; macOS may require you to Control-click the app and choose **Open** the first time.

On Linux, install `zenity` or `kdialog` for file browsing, or enter paths manually. On Windows, the Microsoft Visual C++ runtime may be required.

To build the generator from source, install CMake 3.20+, Git, a C++17 compiler, and Python 3.10+ for the tests. On Linux, also install the X11 and OpenGL development libraries. CMake downloads raylib, raygui, and portable-file-dialogs during configuration.

```sh
cmake -S generator -B generator/build -DCMAKE_BUILD_TYPE=Release
cmake --build generator/build --config Release
ctest --test-dir generator/build -C Release --output-on-failure
```

On Linux or Windows, run `generator/build/distribution/aura-generator` (or `aura-generator.exe` on Windows). To create a distributable directory:

```sh
cmake --install generator/build --config Release --component generator --prefix generator/dist
```

On macOS, the install command creates `generator/dist/Aura Generator.app` with its template included. To create a universal DMG for both Apple Silicon and Intel, configure a separate build directory and package it:

```sh
cmake -S generator -B generator/build-universal -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build generator/build-universal --config Release --target package-dmg
```

The image is `generator/build-universal/aura-generator-macos-universal.dmg`. Packaging verifies that the executable contains both architectures. New macOS builds default to both architectures; existing build directories must be reconfigured or replaced. The DMG is not signed or notarized.

### Cross-build Linux and Windows packages from macOS

Install Docker Desktop, then run this script from the Aura repository root:

```sh
./generator/cross-build.sh
```

The script builds in an `linux/amd64` container, so it also works on Apple Silicon Macs (Docker Desktop may use emulation). It tests the Linux build and cross-compiles the Windows x64 build with MinGW-w64. The `.tar.gz` and `.zip` packages are written to `generator/dist/`; extract each archive and keep its executable beside the `template`, `resources`, and `third-party-licenses` directories. These are x64 packages; this does not produce macOS executables.

In the generator:

1. Enter a project name, application ID, and parent directory.
2. Select your target platforms.
3. If you select Android, choose an installed Android SDK directory. If you select Web, choose an existing Emscripten SDK directory and a Python 3.10+ executable.
4. Select **Generate**.

Aura creates a new project directory; it does not overwrite an existing one. The generated README lists commands for your selected platforms. Emscripten is reused from the selected location, not downloaded or copied.

## Build a generated project

Install JDK 17 or newer and run these commands from the generated project directory:

### Desktop

Install CMake 3.28+, a native compiler, and the required window-system development libraries.

```sh
./gradlew :app:run
./gradlew :app:desktopBuild
```

To cross-compile for Windows from Linux or macOS, install MinGW-w64 and run `./gradlew :app:windowsBuild`. Check `app/src/main/cpp/windows-toolchain.cmake` if your toolchain is installed in a different location.

### Android

Install the Android SDK, NDK, and CMake 4.1.2 through Android Studio. Aura saves the selected SDK path in `local.properties`.

```sh
./gradlew :app:assembleDebug
```

### iOS

On macOS, install Xcode and CMake 3.28+. Run `./gradlew :app:iosBuild` to generate the Xcode project. Configure signing and provisioning in Xcode before building for a device.

The current `iosRun` task invokes `xcodebuild` using `MY_IOS_UDID` from `local.properties`; it does not install or launch the app. Its generated script currently depends on Android being selected as well.

### Web

Install and activate Emscripten, and install Python 3.10+ and CMake. Aura records the selected SDK and Python paths in `app/build.gradle.kts`.

```sh
./gradlew :app:htmlBuild
```

On another machine, override those paths with `-PemsdkDir=/path/to/emsdk -PemsdkPython=/path/to/python3`.

## Project layout

- `app/src/main/cpp/framework/` — shared framework code.
- `app/src/main/cpp/game/` — sample game and screens; start here to build your game.
- `app/src/main/assets/` — game assets.
- `generator/` — native desktop project generator.

Create screens by extending `Framework::Screen` and connect them through `Framework::Game`; see `game/my_game.hpp` and `game/play_screen.hpp` for examples.

## Dependencies and licensing

Aura's original framework and generator code is licensed under the [Apache License 2.0](LICENSE). The license permits commercial and non-commercial use.

Bundled third-party components, including raylib, GLFW, tinyxml2, and the iOS frameworks, remain subject to their own licenses and notices. Review those terms and the permissions for bundled assets before distributing Aura or generated projects.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/tuananhdeveloper)
