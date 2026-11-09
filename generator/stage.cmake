file(MAKE_DIRECTORY "${DEST}/template" "${DEST}/resources" "${DEST}/third-party-licenses")
file(COPY "${ROOT}/app/src/main" DESTINATION "${DEST}/template/app/src"
    PATTERN "emsdk" EXCLUDE PATTERN "build" EXCLUDE PATTERN "tests" EXCLUDE)
file(COPY "${ROOT}/gradle" DESTINATION "${DEST}/template"
    PATTERN "gradle-daemon-jvm.properties" EXCLUDE)
file(COPY "${ROOT}/LICENSE" "${ROOT}/gradlew" "${ROOT}/gradlew.bat"
    "${ROOT}/gradle.properties" "${ROOT}/build.gradle.kts" "${ROOT}/settings.gradle.kts"
    DESTINATION "${DEST}/template")
file(COPY "${ROOT}/generator/src/main/resources/" DESTINATION "${DEST}/resources")
configure_file("${RAYLIB_LICENSE}" "${DEST}/third-party-licenses/raylib.txt" COPYONLY)
configure_file("${RAYGUI_LICENSE}" "${DEST}/third-party-licenses/raygui.txt" COPYONLY)
configure_file("${PFD_LICENSE}" "${DEST}/third-party-licenses/portable-file-dialogs.txt" COPYONLY)
