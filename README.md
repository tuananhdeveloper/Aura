# Aura

Aura is a Java desktop application that generates a raylib starter project for Android, iOS, desktop, and web. Choose the project name, application ID, destination, and target platforms in the generator.

## Run the generator

Install JDK 17 or newer, then run:

```sh
./gradlew -p generator jar
java -jar generator/build/libs/raylib-starter-generator.jar
```

The generator creates a new directory and will not overwrite an existing one. Generated projects include a README with the prerequisites and build commands for the selected platforms.

For Web, provide the absolute path to Python 3.10 or newer. Git and internet access are also required: Aura clones the Emscripten SDK into `app/src/main/cpp/emsdk`, then downloads and activates its latest toolchain. This can take substantial time and disk space. SDK configuration is machine-specific; reactivate it after moving the generated project.

If generation fails, the incomplete output directory is left in place for inspection. Remove or rename it before retrying.

## Build generated projects

- **Desktop:** `./gradlew :app:run` or `./gradlew :app:desktopBuild`.
- **Android:** Configure the Android SDK, NDK, and CMake in Android Studio, then run `./gradlew :app:assembleDebug`.
- **iOS:** On macOS with Xcode, run `./gradlew :app:iosBuild -PiosTeam=<team-ID>`, or `./gradlew :app:iosRun -PiosTeam=<team-ID> -PiosDevice=<device-UDID>` for a connected, provisioned device.
- **Web:** Run `./gradlew :app:htmlBuild`. Configure the Emscripten and Python paths in `app/build.gradle.kts` or override them with `-PemsdkDir` and `-PemsdkPython`.

The native project in `app/src/main` provides the starter template. The Emscripten SDK is not included in this repository.
