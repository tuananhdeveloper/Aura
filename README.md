# Aura

Aura is a Java desktop application that generates a raylib starter project for Android, iOS, desktop, and web. Choose the project name, application ID, destination, and target platforms in the generator.

## Run the generator

Install JDK 17 or newer, then run:

```sh
./gradlew -p generator jar
java -jar generator/build/libs/raylib-starter-generator.jar
```

The generator creates a new directory and will not overwrite an existing one. Generated projects include a README with the prerequisites and build commands for the selected platforms.
The project name also becomes the native CMake target and Android library name. Names reserved by CMake or already used by bundled native libraries cannot be used.

For Web, select an existing Emscripten SDK directory and provide the absolute path to Python 3.10 or newer. Aura reuses that installation; it does not download, copy, or activate the SDK while generating projects. The generated Gradle file points to the selected SDK and Python paths, which can be overridden with `-PemsdkDir=/path/to/emsdk -PemsdkPython=/path/to/python3`. These paths are machine-specific.

If generation fails, the incomplete output directory is left in place for inspection. Remove or rename it before retrying.

## Build generated projects

- **Desktop:** `./gradlew :app:run` or `./gradlew :app:desktopBuild`.
- **Android:** Configure the Android SDK, NDK, and CMake in Android Studio, then run `./gradlew :app:assembleDebug`.
- **iOS:** On macOS with Xcode, run `./gradlew :app:iosBuild -PiosTeam=<team-ID>`, or `./gradlew :app:iosRun -PiosTeam=<team-ID> -PiosDevice=<device-UDID>` for a connected, provisioned device.
- **Web:** Run `./gradlew :app:htmlBuild`. The selected Emscripten SDK and Python paths are configured in `app/build.gradle.kts`; override them with `-PemsdkDir` and `-PemsdkPython` if needed.

The native project in `app/src/main` provides the starter template. The Emscripten SDK is not included in this repository.
