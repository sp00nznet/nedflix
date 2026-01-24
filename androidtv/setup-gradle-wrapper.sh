#!/bin/bash
# Downloads the Gradle wrapper JAR if it doesn't exist

WRAPPER_DIR="gradle/wrapper"
WRAPPER_JAR="$WRAPPER_DIR/gradle-wrapper.jar"
GRADLE_VERSION="8.4"

# Verify if existing JAR is valid (should be a real JAR file, not HTML)
if [ -f "$WRAPPER_JAR" ]; then
    if ! unzip -t "$WRAPPER_JAR" >/dev/null 2>&1; then
        echo "Existing gradle-wrapper.jar is invalid, re-downloading..."
        rm -f "$WRAPPER_JAR"
    fi
fi

if [ ! -f "$WRAPPER_JAR" ]; then
    echo "Downloading Gradle wrapper JAR..."
    mkdir -p "$WRAPPER_DIR"

    # Download from Maven Central (most reliable source)
    WRAPPER_URL="https://repo1.maven.org/maven2/org/gradle/gradle-wrapper/${GRADLE_VERSION}/gradle-wrapper-${GRADLE_VERSION}.jar"

    if command -v curl >/dev/null 2>&1; then
        curl -L -f -o "$WRAPPER_JAR" "$WRAPPER_URL"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "$WRAPPER_JAR" "$WRAPPER_URL"
    else
        echo "ERROR: Neither curl nor wget is available to download gradle-wrapper.jar"
        exit 1
    fi

    # Verify the download is a valid JAR
    if [ -f "$WRAPPER_JAR" ] && unzip -t "$WRAPPER_JAR" >/dev/null 2>&1; then
        echo "Gradle wrapper JAR downloaded and verified successfully"
    else
        echo "ERROR: Failed to download valid gradle-wrapper.jar"
        rm -f "$WRAPPER_JAR"
        exit 1
    fi
else
    echo "Gradle wrapper JAR already exists and is valid"
fi
