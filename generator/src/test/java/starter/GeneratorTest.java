package starter;

import java.io.InputStream;
import java.nio.file.*;
import java.util.*;

public final class GeneratorTest {
    public static void main(String[] args) throws Exception {
        Path parent = Files.createTempDirectory("starter-generator-test");
        try {
            run(parent);
        } finally {
            try (var paths = Files.walk(parent)) {
                for (Path path : paths.sorted(Comparator.reverseOrder()).toList()) Files.delete(path);
            }
        }
    }

    private static void run(Path parent) throws Exception {
        var pythonChooser = StarterGenerator.pythonFileChooser(parent);
        check(pythonChooser.getFileSelectionMode() == javax.swing.JFileChooser.FILES_ONLY,
                "Python chooser selects files");
        check(pythonChooser.getFileFilter().accept(parent.resolve("python3.12").toFile()),
                "Python chooser accepts versioned Python executable");
        check(!pythonChooser.getFileFilter().accept(parent.resolve("notes.txt").toFile()),
                "Python chooser filters unrelated files");

        var desktop = new StarterGenerator.Options("DesktopGame", "com.example.desktop",
                parent.resolve("DesktopGame"), EnumSet.of(StarterGenerator.Platform.DESKTOP), null, null);
        generate(desktop);
        String app = Files.readString(desktop.output().resolve("app/build.gradle.kts"));
        check(app.contains("\"desktopBuild\"") && !app.contains("\"htmlBuild\"") && !app.contains("android {"),
                "Only selected tasks are generated");
        check(!Files.exists(desktop.output().resolve("app/src/main/cpp/emsdk")), "SDK not copied");
        check(!Files.exists(desktop.output().resolve("app/src/main/cpp/ios-frameworks")), "iOS frameworks not copied");
        check(Files.isRegularFile(desktop.output().resolve("app/src/main/cpp/raylib/src/raylib.h")), "raylib copied");
        check(Files.readString(desktop.output().resolve("settings.gradle.kts")).contains("\"DesktopGame\""), "project name");
        check(Files.readString(desktop.output().resolve("app/src/main/cpp/CMakeLists.txt"))
                .contains("project(DesktopGame LANGUAGES C CXX)"), "CMake project name");
        check(Files.readString(desktop.output().resolve("app/src/main/cpp/CMakeLists.txt"))
                .contains("add_executable(DesktopGame main.cpp)"), "desktop target name");
        check(app.contains("\"./\" + rootProject.name"), "desktop runs case-sensitive target name");
        String readme = Files.readString(desktop.output().resolve("README.md"));
        check(readme.contains("# DesktopGame") && readme.contains("`./gradlew :app:run`"),
                "README includes the project name and desktop command");
        check(!readme.contains("### Android") && !readme.contains("### Web"), "README lists selected platforms only");

        try {
            generate(desktop);
            throw new AssertionError("Existing output was accepted");
        } catch (IllegalArgumentException expected) {
            check(Files.exists(desktop.output().resolve("gradlew")), "existing output preserved");
        }
        try {
            StarterGenerator.validate(new StarterGenerator.Options("../bad", "com.example.game",
                    parent.resolve("bad"), EnumSet.of(StarterGenerator.Platform.DESKTOP), null, null));
            throw new AssertionError("Invalid name accepted");
        } catch (IllegalArgumentException expected) {
            // Expected.
        }
        try {
            StarterGenerator.validate(new StarterGenerator.Options("raylib", "com.example.game",
                    parent.resolve("raylib"), EnumSet.of(StarterGenerator.Platform.DESKTOP), null, null));
            throw new AssertionError("Conflicting native target name accepted");
        } catch (IllegalArgumentException expected) {
            // Expected.
        }

        Path python = findPython();
        if (python == null) throw new AssertionError("Python 3.10+ is required to exercise web generation");
        Path sdk = Files.createDirectory(parent.resolve("shared emsdk"));
        Files.writeString(sdk.resolve("emsdk_env.sh"), "#!/bin/sh\n");
        var all = new StarterGenerator.Options("AllPlatforms", "com.example.all",
                parent.resolve("AllPlatforms"), EnumSet.allOf(StarterGenerator.Platform.class), sdk, python);
        generate(all);
        check(!Files.exists(all.output().resolve("app/src/main/cpp/emsdk")), "SDK not copied into generated project");
        app = Files.readString(all.output().resolve("app/build.gradle.kts"));
        for (String task : List.of("desktopBuild", "iosBuild", "iosRun", "htmlBuild"))
            check(app.contains("\"" + task + "\""), "missing " + task);
        check(app.contains("namespace = \"com.example.all\""), "Android application ID");
        check(Files.readString(all.output().resolve("app/src/main/cpp/CMakeLists.txt"))
                .contains("com.example.all"), "iOS application ID");
        check(Files.readString(all.output().resolve("app/src/main/cpp/CMakeLists.txt"))
                .contains("project(AllPlatforms LANGUAGES C CXX)"), "CMake project name for all platforms");
        String cmake = Files.readString(all.output().resolve("app/src/main/cpp/CMakeLists.txt"));
        check(!cmake.contains("myraylibgame"), "old CMake target name removed");
        check(cmake.contains("add_library(AllPlatforms SHARED main.cpp)"), "Android target name");
        check(cmake.contains("add_executable(AllPlatforms main.cpp)"), "native executable target name");
        check(Files.readString(all.output().resolve("app/src/main/AndroidManifest.xml"))
                .contains("android:value=\"AllPlatforms\""), "Android NativeActivity library name");
        check(app.contains("\"-scheme\", rootProject.name"), "iOS scheme matches target name");
        check(app.contains("${iosBuildDir}/${rootProject.name}.xcodeproj"), "iOS project matches CMake project name");
        check(app.contains(sdk.toAbsolutePath().toString()), "selected SDK path configured");
        check(app.contains(python.toAbsolutePath().toString()), "selected Python path configured");
        check(app.contains("Emscripten SDK directory not found"), "SDK path validated at build time");
        check(app.contains("cmd\", \"/d\", \"/c\""), "Windows SDK environment supported");
        check(app.contains("source ") && app.contains("emcmake cmake"), "Unix SDK environment supported");
        readme = Files.readString(all.output().resolve("README.md"));
        check(readme.contains("### Android") && readme.contains("### iOS")
                && readme.contains("### Desktop") && readme.contains("### Web"),
                "README includes selected platforms");
        check(readme.contains("MY_IOS_UDID") && readme.contains("Python 3.10+"),
                "README includes concise platform setup");
        check(Files.readString(all.output().resolve("app/src/main/cpp/CMakeLists.txt"))
                .contains("enable_language(OBJC)"), "iOS Objective-C enabled");
        check(Files.readString(all.output().resolve("app/src/main/cpp/main.cpp"))
                .contains("#include <TargetConditionals.h>"), "Apple target macros available");
        check(Files.isDirectory(all.output().resolve("app/src/main/cpp/ios-frameworks")), "iOS frameworks copied");
        if (!System.getProperty("os.name").startsWith("Windows"))
            check(Files.isExecutable(all.output().resolve("app/src/main/cpp/ios-frameworks/libEGL.xcframework/ios-arm64/libEGL.framework/libEGL")),
                    "iOS framework binary executable");

        try {
            StarterGenerator.validate(new StarterGenerator.Options("MissingSdk", "com.example.fail",
                    parent.resolve("MissingSdk"), EnumSet.of(StarterGenerator.Platform.WEB), parent.resolve("missing"), python));
            throw new AssertionError("Missing SDK accepted");
        } catch (IllegalArgumentException expected) {
            check(expected.getMessage().contains("existing Emscripten SDK"), "Missing SDK error");
        }
        Path invalidSdk = Files.createDirectory(parent.resolve("not-an-sdk"));
        try {
            StarterGenerator.validate(new StarterGenerator.Options("InvalidSdk", "com.example.fail",
                    parent.resolve("InvalidSdk"), EnumSet.of(StarterGenerator.Platform.WEB), invalidSdk, python));
            throw new AssertionError("Directory without an Emscripten environment script accepted");
        } catch (IllegalArgumentException expected) {
            check(expected.getMessage().contains("not an Emscripten SDK"), "Invalid SDK error");
        }
    }

    private static void generate(StarterGenerator.Options options) throws Exception {
        try (InputStream template = GeneratorTest.class.getResourceAsStream("/starter-template.zip")) {
            if (template == null) throw new AssertionError("template missing");
            StarterGenerator.generate(options, template, line -> {});
        }
    }

    private static Path findPython() {
        for (String dir : System.getenv().getOrDefault("PATH", "").split(java.io.File.pathSeparator)) {
            for (String name : List.of("python3", "python.exe")) {
                Path path = Path.of(dir, name).toAbsolutePath();
                if (!Files.isRegularFile(path)) continue;
                try {
                    if (new ProcessBuilder(path.toString(), "-c", "import sys; sys.exit(sys.version_info < (3,10))")
                            .start().waitFor() == 0) return path;
                } catch (Exception ignored) {
                    // Try another executable.
                }
            }
        }
        return null;
    }

    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
}
