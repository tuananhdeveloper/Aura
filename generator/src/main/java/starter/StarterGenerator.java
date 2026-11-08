package starter;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.filechooser.FileFilter;
import java.awt.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.nio.file.attribute.PosixFilePermission;
import java.util.*;
import java.util.List;
import java.util.function.Consumer;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public final class StarterGenerator {
    public static final String APP_NAME = "Aura";

    enum Platform { ANDROID, IOS, DESKTOP, WEB }

    record Options(String name, String applicationId, Path output, Set<Platform> platforms,
                   Path androidSdkDir, Path emsdkDir, Path python) {}

    static void validate(Options options) throws IOException, InterruptedException {
        if (options.platforms().contains(Platform.ANDROID)) {
            String sdkError = androidSdkError(options.androidSdkDir());
            if (sdkError != null) throw new IllegalArgumentException(sdkError);
        }
        if (!options.name().matches("[A-Za-z][A-Za-z0-9_-]{0,63}"))
            throw new IllegalArgumentException("Project name must start with a letter and contain only letters, digits, - and _ (maximum 64 characters).");
        if (Set.of("raylib", "native_app_glue", "glfw", "all", "clean", "help", "install",
                "test", "package", "rebuild_cache", "edit_cache", "list_install_components",
                "ZERO_CHECK", "ALL_BUILD", "RUN_TESTS").contains(options.name()))
            throw new IllegalArgumentException("Project name conflicts with a native build target: " + options.name());
        if (!options.applicationId().matches("[a-z][a-z0-9_]*(\\.[a-z][a-z0-9_]*)+"))
            throw new IllegalArgumentException("Application ID must contain lowercase identifier segments, for example com.example.game.");
        if (options.platforms().isEmpty())
            throw new IllegalArgumentException("Select at least one platform.");
        if (Files.exists(options.output()))
            throw new IllegalArgumentException("Output already exists: " + options.output());
        if (options.output().getParent() == null || !Files.isDirectory(options.output().getParent()))
            throw new IllegalArgumentException("The output parent directory must exist.");
        if (options.platforms().contains(Platform.WEB)) {
            if (options.emsdkDir() == null || !Files.isDirectory(options.emsdkDir()))
                throw new IllegalArgumentException("Select an existing Emscripten SDK directory.");
            if (!isEmscriptenSdk(options.emsdkDir()))
                throw new IllegalArgumentException("Selected directory is not an Emscripten SDK: " + options.emsdkDir());
            if (options.python() == null || !Files.isRegularFile(options.python()))
                throw new IllegalArgumentException("Select a Python 3.10+ executable for the web SDK.");
            int version = new ProcessBuilder(options.python().toString(), "-c",
                    "import sys; sys.exit(0 if sys.version_info >= (3,10) else 1)")
                    .redirectErrorStream(true).redirectOutput(ProcessBuilder.Redirect.DISCARD)
                    .start().waitFor();
            if (version != 0)
                throw new IllegalArgumentException("Python 3.10 or newer is required: " + options.python());
        }
    }

    static String androidSdkError(Path directory) {
        if (directory == null || !Files.isDirectory(directory))
            return "Select an existing Android SDK directory.";
        boolean hasAdb = Files.isRegularFile(directory.resolve("platform-tools/adb"))
                || Files.isRegularFile(directory.resolve("platform-tools/adb.exe"));
        if (!hasAdb || !Files.isDirectory(directory.resolve("platforms")))
            return "The selected directory is not an Android SDK. Choose the SDK root containing platform-tools and platforms.";
        return null;
    }

    static boolean isEmscriptenSdk(Path directory) {
        boolean hasEnvironment = Files.isRegularFile(directory.resolve("emsdk_env.sh"))
                || Files.isRegularFile(directory.resolve("emsdk_env.bat"));
        boolean hasEmcc = Files.isRegularFile(directory.resolve("upstream/emscripten/emcc"))
                || Files.isRegularFile(directory.resolve("upstream/emscripten/emcc.bat"));
        boolean hasClang = Files.isRegularFile(directory.resolve("upstream/bin/clang"))
                || Files.isRegularFile(directory.resolve("upstream/bin/clang.exe"));
        return hasEnvironment && hasEmcc && hasClang;
    }

    static void generate(Options options, InputStream template, Consumer<String> log)
            throws IOException, InterruptedException {
        validate(options);
        Path output = options.output().toAbsolutePath().normalize();
        Files.createDirectory(output);
        log.accept("Creating project in " + output);
        try (ZipInputStream zip = new ZipInputStream(template)) {
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                String name = entry.getName();
                if (!options.platforms().contains(Platform.IOS)
                        && name.startsWith("app/src/main/cpp/ios-frameworks/")) continue;
                Path target = output.resolve(name).normalize();
                if (!target.startsWith(output)) throw new IOException("Invalid template path: " + name);
                if (entry.isDirectory()) {
                    Files.createDirectories(target);
                } else {
                    Files.createDirectories(target.getParent());
                    Files.copy(zip, target);
                    Path parent = target.getParent();
                    if (parent.getFileName().toString().equals(target.getFileName() + ".framework"))
                        makeExecutable(target);
                }
            }
        }
        makeExecutable(output.resolve("gradlew"));

        String app = resource("app.gradle.kts")
                .replace("@@ANDROID@@", fragment(options, Platform.ANDROID, "android.gradle.kts"))
                .replace("@@DESKTOP@@", fragment(options, Platform.DESKTOP, "desktop.gradle.kts"))
                .replace("@@IOS@@", fragment(options, Platform.IOS, "ios.gradle.kts"))
                .replace("@@WEB@@", fragment(options, Platform.WEB, "web.gradle.kts"))
                .replace("@@PACKAGE@@", options.applicationId())
                .replace("@@PYTHON@@", options.python() == null ? "" : kotlinString(options.python().toAbsolutePath().toString()))
                .replace("@@EMSDK@@", options.emsdkDir() == null ? "" : kotlinString(options.emsdkDir().toAbsolutePath().toString()));
        write(output.resolve("app/build.gradle.kts"), app);
        Path settings = output.resolve("settings.gradle.kts");
        if (options.platforms().contains(Platform.ANDROID)) {
            write(settings, Files.readString(settings).replace("@@NAME@@", options.name));
        } else {
            write(settings, "rootProject.name = \"" + options.name() + "\"\ninclude(\":app\")\n");
            write(output.resolve("build.gradle.kts"), "");
        }
        Path strings = output.resolve("app/src/main/res/values/strings.xml");
        write(strings, Files.readString(strings).replace("@@APP_NAME@@", options.name()));
        Path cmake = output.resolve("app/src/main/cpp/CMakeLists.txt");
        write(cmake, Files.readString(cmake)
                .replace("@@NAME@@", options.name())
                .replace("@@PACKAGE@@", options.applicationId())
                .replace("elseif (CMAKE_SYSTEM_NAME STREQUAL \"iOS\")",
                        "elseif (CMAKE_SYSTEM_NAME STREQUAL \"iOS\")\n    enable_language(OBJC)"));
        Path manifest = output.resolve("app/src/main/AndroidManifest.xml");
        write(manifest, Files.readString(manifest)
                .replace("@@NAME@@", options.name()));
        Path preferences = output.resolve("app/src/main/cpp/framework/preferences.hpp");
        write(preferences, Files.readString(preferences)
                .replace("@@NAME@@", options.name()));
        if (options.platforms().contains(Platform.ANDROID)) {
            Properties localProperties = new Properties();
            localProperties.setProperty("sdk.dir", options.androidSdkDir().toAbsolutePath().toString());
            try (Writer writer = Files.newBufferedWriter(output.resolve("local.properties"), StandardCharsets.ISO_8859_1)) {
                localProperties.store(writer, "Android SDK location");
            }
        }
        Path main = output.resolve("app/src/main/cpp/main.cpp");
        write(main, Files.readString(main).replace("@@NAME@@", options.name()));
        for (String file : List.of("app/src/main/cpp/main.cpp", "app/src/main/cpp/framework/platform.hpp")) {
            Path source = output.resolve(file);
            write(source, "#if defined(__APPLE__)\n#include <TargetConditionals.h>\n#endif\n" + Files.readString(source));
        }
        Path properties = output.resolve("gradle.properties");
        write(properties, Files.readString(properties).replace("org.gradle.configuration-cache=true",
                "org.gradle.configuration-cache=false"));
        write(output.resolve(".gitignore"), ".gradle/\n**/build/\nlocal.properties\n.cxx/\n.externalNativeBuild/\napp/src/main/cpp/emsdk/\n");
        write(output.resolve("README.md"), documentation(options));

        log.accept("Project generated successfully.");
    }

    private static void makeExecutable(Path file) throws IOException {
        try {
            Set<PosixFilePermission> permissions = Files.getPosixFilePermissions(file);
            permissions.add(PosixFilePermission.OWNER_EXECUTE);
            permissions.add(PosixFilePermission.GROUP_EXECUTE);
            permissions.add(PosixFilePermission.OTHERS_EXECUTE);
            Files.setPosixFilePermissions(file, permissions);
        } catch (UnsupportedOperationException ignored) {
            // Windows does not support POSIX file permissions.
        }
    }

    private static String fragment(Options options, Platform platform, String file) throws IOException {
        return options.platforms().contains(platform) ? resource(file) : "";
    }

    private static String resource(String name) throws IOException {
        try (InputStream in = StarterGenerator.class.getResourceAsStream("/" + name)) {
            if (in == null) throw new IOException("Generator resource missing: " + name);
            return new String(in.readAllBytes(), StandardCharsets.UTF_8);
        }
    }

    private static void write(Path path, String content) throws IOException {
        Files.writeString(path, content, StandardCharsets.UTF_8);
    }

    private static String kotlinString(String value) {
        return value.replace("\\", "\\\\").replace("\"", "\\\"").replace("$", "\\$");
    }

    static String documentation(Options options) {
        StringBuilder text = new StringBuilder("# ").append(options.name())
                .append("\n\nA starter project.\n\n## Build\n");
        if (options.platforms().contains(Platform.DESKTOP)) {
            text.append("\n### Desktop\nRequires CMake 3.28+ and a native compiler.\n")
                    .append("- Run: `./gradlew :app:run`\n")
                    .append("- Build only: `./gradlew :app:desktopBuild`\n")
                    .append("- Windows cross-build (Linux/macOS): `./gradlew :app:windowsBuild` (requires MinGW-w64)\n");
        }
        if (options.platforms().contains(Platform.ANDROID)) {
            text.append("\n### Android\nRequires Android Studio with the Android SDK, NDK, and CMake installed. The SDK path is saved in `local.properties`.\n")
                    .append("- Build: `./gradlew :app:assembleDebug`\n");
        }
        if (options.platforms().contains(Platform.IOS)) {
            text.append("\n### iOS\nRequires macOS, Xcode, and CMake 3.28+.\n")
                    .append("- Build: `./gradlew :app:iosBuild`\n")
                    .append("- Run: `./gradlew :app:iosRun` (set `MY_IOS_UDID` in `local.properties`)\n");
        }
        if (options.platforms().contains(Platform.WEB)) {
            text.append("\n### Web\nRequires an installed Emscripten SDK, Python 3.10+, and CMake. ")
                    .append("Set the SDK and Python paths in `app/build.gradle.kts`.\n")
                    .append("- Build: `./gradlew :app:htmlBuild`\n");
        }
        return text.toString();
    }

    private static void showWindow() {
        JFrame frame = new JFrame(APP_NAME);
        frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        JPanel form = new JPanel(new GridBagLayout());
        form.setBorder(new EmptyBorder(12, 12, 12, 12));
        GridBagConstraints c = new GridBagConstraints();
        c.insets = new Insets(4, 4, 4, 4);
        c.fill = GridBagConstraints.HORIZONTAL;
        c.weightx = 1;
        JTextField name = new JTextField("MyGame", 30);
        JTextField packageName = new JTextField("com.example.mygame", 30);
        JTextField parent = new JTextField(System.getProperty("user.home"), 30);
        JTextField androidSdk = new JTextField(30);
        androidSdk.setEditable(false);
        JTextField emsdk = new JTextField(30);
        emsdk.setEditable(false);
        JTextField python = new JTextField(30);
        python.setEditable(false);
        JButton browse = new JButton("Browse...");
        browse.addActionListener(event -> {
            JFileChooser chooser = new JFileChooser(parent.getText());
            chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
            if (chooser.showOpenDialog(frame) == JFileChooser.APPROVE_OPTION)
                parent.setText(chooser.getSelectedFile().getAbsolutePath());
        });
        JPanel location = new JPanel(new BorderLayout(4, 0));
        location.add(parent);
        location.add(browse, BorderLayout.EAST);
        JButton browseAndroidSdk = new JButton("Browse...");
        browseAndroidSdk.addActionListener(event -> {
            JFileChooser chooser = new JFileChooser(androidSdk.getText().isBlank()
                    ? System.getProperty("user.home") : androidSdk.getText());
            chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
            if (chooser.showOpenDialog(frame) == JFileChooser.APPROVE_OPTION)
                androidSdk.setText(chooser.getSelectedFile().getAbsolutePath());
        });
        JPanel androidSdkLocation = new JPanel(new BorderLayout(4, 0));
        androidSdkLocation.add(androidSdk);
        androidSdkLocation.add(browseAndroidSdk, BorderLayout.EAST);
        JButton browseEmsdk = new JButton("Browse...");
        browseEmsdk.addActionListener(event -> {
            JFileChooser chooser = new JFileChooser(emsdk.getText().isBlank() ? System.getProperty("user.home") : emsdk.getText());
            chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
            if (chooser.showOpenDialog(frame) == JFileChooser.APPROVE_OPTION)
                emsdk.setText(chooser.getSelectedFile().getAbsolutePath());
        });
        JPanel emsdkLocation = new JPanel(new BorderLayout(4, 0));
        emsdkLocation.add(emsdk);
        emsdkLocation.add(browseEmsdk, BorderLayout.EAST);
        JButton browsePython = new JButton("Browse...");
        browsePython.addActionListener(event -> {
            Path initialDirectory = python.getText().isBlank()
                    ? Path.of(System.getProperty("user.home"))
                    : Path.of(python.getText()).getParent();
            JFileChooser chooser = pythonFileChooser(initialDirectory);
            if (chooser.showOpenDialog(frame) == JFileChooser.APPROVE_OPTION)
                python.setText(chooser.getSelectedFile().getAbsolutePath());
        });
        JPanel pythonLocation = new JPanel(new BorderLayout(4, 0));
        pythonLocation.add(python);
        pythonLocation.add(browsePython, BorderLayout.EAST);
        JPanel platforms = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        Map<Platform, JCheckBox> selections = new EnumMap<>(Platform.class);
        for (Platform platform : Platform.values()) {
            JCheckBox box = new JCheckBox(switch (platform) {
                case ANDROID -> "Android"; case IOS -> "iOS"; case DESKTOP -> "Desktop"; case WEB -> "Web";
            }, platform == Platform.DESKTOP);
            selections.put(platform, box);
            platforms.add(box);
        }
        addRow(form, c, 0, "Project name", name);
        addRow(form, c, 1, "Application ID", packageName);
        addRow(form, c, 2, "Parent directory", location);
        addRow(form, c, 3, "Platforms", platforms);
        addRow(form, c, 4, "Android SDK (Android)", androidSdkLocation);
        addRow(form, c, 5, "Existing Emscripten SDK (Web)", emsdkLocation);
        addRow(form, c, 6, "Python executable (Web)", pythonLocation);
        JTextArea log = new JTextArea(12, 60);
        log.setEditable(false);
        JButton generate = new JButton("Generate");
        generate.addActionListener(event -> {
            try {
                EnumSet<Platform> selected = EnumSet.noneOf(Platform.class);
                selections.forEach((platform, box) -> { if (box.isSelected()) selected.add(platform); });
                if (selected.contains(Platform.ANDROID)) {
                    String sdkError = androidSdkError(androidSdk.getText().isBlank()
                            ? null : Path.of(androidSdk.getText().trim()));
                    if (sdkError != null) {
                    JOptionPane.showMessageDialog(frame,
                            sdkError,
                            "Android SDK required", JOptionPane.ERROR_MESSAGE);
                    return;
                    }
                }
                String projectName = name.getText().trim();
                Path destination = Path.of(parent.getText()).toAbsolutePath().resolve(projectName).normalize();
                Options options = new Options(projectName, packageName.getText().trim(), destination, selected,
                        androidSdk.getText().isBlank() ? null : Path.of(androidSdk.getText().trim()).toAbsolutePath(),
                        emsdk.getText().isBlank() ? null : Path.of(emsdk.getText().trim()).toAbsolutePath(),
                        python.getText().isBlank() ? null : Path.of(python.getText().trim()).toAbsolutePath());
                generate.setEnabled(false);
                new SwingWorker<Void, String>() {
                    protected Void doInBackground() throws Exception {
                        try (InputStream template = StarterGenerator.class.getResourceAsStream("/starter-template.zip")) {
                            if (template == null) throw new IOException("Bundled starter template is missing.");
                            StarterGenerator.generate(options, template, this::publish);
                        }
                        return null;
                    }
                    protected void process(List<String> lines) {
                        lines.forEach(line -> log.append(line + "\n"));
                        log.setCaretPosition(log.getDocument().getLength());
                    }
                    protected void done() {
                        generate.setEnabled(true);
                        try {
                            get();
                        } catch (Exception ex) {
                            Throwable cause = ex.getCause() == null ? ex : ex.getCause();
                            log.append("Error: " + cause.getMessage() + "\nIf generation started, the incomplete output was left in place.\n");
                            JOptionPane.showMessageDialog(frame, cause.getMessage(), "Generation failed", JOptionPane.ERROR_MESSAGE);
                        }
                    }
                }.execute();
            } catch (RuntimeException ex) {
                JOptionPane.showMessageDialog(frame, ex.getMessage(), "Invalid input", JOptionPane.ERROR_MESSAGE);
            }
        });
        JPanel content = new JPanel(new BorderLayout(4, 4));
        content.add(form, BorderLayout.NORTH);
        content.add(new JScrollPane(log), BorderLayout.CENTER);
        content.add(generate, BorderLayout.SOUTH);
        frame.setContentPane(content);
        frame.pack();
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }

    static JFileChooser pythonFileChooser(Path initialDirectory) {
        JFileChooser chooser = new JFileChooser(initialDirectory.toFile());
        chooser.setFileSelectionMode(JFileChooser.FILES_ONLY);
        chooser.setAcceptAllFileFilterUsed(false);
        chooser.setFileFilter(new FileFilter() {
            @Override
            public boolean accept(File file) {
                return file.isDirectory() || file.getName().toLowerCase(Locale.ROOT).startsWith("python");
            }

            @Override
            public String getDescription() {
                return "Python executable";
            }
        });
        return chooser;
    }

    private static void addRow(JPanel panel, GridBagConstraints c, int row, String label, Component field) {
        c.gridy = row;
        c.gridx = 0;
        c.weightx = 0;
        panel.add(new JLabel(label), c);
        c.gridx = 1;
        c.weightx = 1;
        panel.add(field, c);
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(StarterGenerator::showWindow);
    }
}
