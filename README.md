# Aura

Aura is a C++ game framework built on [raylib](https://www.raylib.com/), with a Java desktop generator for creating starter projects. It targets Android, iOS, desktop, and the web.

The starter includes a sample game and shared framework components for screens and lifecycle, input, viewports, assets, preferences, UI, tilemaps, tweens, and particles.

## Create a project

Install JDK 17 or newer, then build and open the generator from this repository:

```sh
./gradlew -p generator jar
java -jar generator/build/libs/aura-starter-generator.jar
```

In the generator:

1. Enter a project name, application ID, and parent directory.
2. Select your target platforms.
3. If you select Android, choose an installed Android SDK directory. If you select Web, choose an existing Emscripten SDK directory and a Python 3.10+ executable.
4. Select **Generate**.

Aura creates a new project directory; it does not overwrite an existing one. The generated README lists commands for your selected platforms. Emscripten is reused from the selected location, not downloaded or copied.

## Build a generated project

Run these commands from the generated project directory:

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
- `generator/` — Java desktop project generator.

Create screens by extending `Framework::Screen` and connect them through `Framework::Game`; see `game/my_game.hpp` and `game/play_screen.hpp` for examples.

## Dependencies and licensing

Aura's original framework and generator code is licensed under the [Apache License 2.0](LICENSE). The license permits commercial and non-commercial use.

Bundled third-party components, including raylib, GLFW, tinyxml2, and the iOS frameworks, remain subject to their own licenses and notices. Review those terms and the permissions for bundled assets before distributing Aura or generated projects.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/tuananhdeveloper)
