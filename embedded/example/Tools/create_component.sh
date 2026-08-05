#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 ComponentName [file_name]" >&2
    echo "Example: $0 Button button" >&2
    echo "Example: $0 CanProtocol can_protocol" >&2
    exit 1
fi

COMPONENT_NAME="$1"
FILE_INPUT="${2:-}"

if [ -z "$FILE_INPUT" ]; then
    FILE_NAME="$(
        printf '%s' "$COMPONENT_NAME" |
            tr '[:upper:]' '[:lower:]'
    )"
else
    case "$FILE_INPUT" in
        *.c | *.h)
            FILE_NAME="${FILE_INPUT%.*}"
            ;;
        *)
            FILE_NAME="$FILE_INPUT"
            ;;
    esac
fi

case "$COMPONENT_NAME" in
    '' | *[!A-Za-z0-9_]*)
        echo "Error: ComponentName may contain only letters, digits, and underscores." >&2
        exit 1
        ;;
esac

case "$COMPONENT_NAME" in
    [A-Za-z_]*)
        ;;
    *)
        echo "Error: ComponentName cannot start with a digit." >&2
        exit 1
        ;;
esac

case "$FILE_NAME" in
    '' | *[!a-z0-9_]*)
        echo "Error: file_name may contain only lowercase letters, digits, and underscores." >&2
        exit 1
        ;;
esac

case "$FILE_NAME" in
    [a-z_]*)
        ;;
    *)
        echo "Error: file_name cannot start with a digit." >&2
        exit 1
        ;;
esac

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq is required to read the VS Code snippet." >&2
    exit 1
fi

ROOT_DIR="$(
    cd "$(dirname "${BASH_SOURCE[0]}")/.." &&
        pwd
)"

COMPONENTS_DIR="$ROOT_DIR/Components"
COMPONENT_DIR="$COMPONENTS_DIR/$COMPONENT_NAME"
TESTS_DIR="$COMPONENT_DIR/Tests"

HEADER_FILE="$COMPONENT_DIR/$FILE_NAME.h"
SOURCE_FILE="$COMPONENT_DIR/$FILE_NAME.c"
TEST_FILE="$TESTS_DIR/test_$FILE_NAME.c"
CMAKE_FILE="$COMPONENT_DIR/CMakeLists.txt"

SNIPPET_FILE="$ROOT_DIR/.vscode/c-header.code-snippets"
SNIPPET_NAME="C header file"

COMPONENT_TARGET="component_$FILE_NAME"
TEST_TARGET="test_$FILE_NAME"

HEADER_GUARD="$(
    printf '%s' "$FILE_NAME" |
        tr '[:lower:]' '[:upper:]'
)"

if [ ! -f "$COMPONENTS_DIR/CMakeLists.txt" ]; then
    echo "Error: Components/CMakeLists.txt not found." >&2
    exit 1
fi

if [ ! -f "$SNIPPET_FILE" ]; then
    echo "Error: snippet file not found: $SNIPPET_FILE" >&2
    exit 1
fi

if ! jq empty "$SNIPPET_FILE" >/dev/null 2>&1; then
    echo "Error: snippet file is not valid JSON: $SNIPPET_FILE" >&2
    exit 1
fi

if ! jq -e \
    --arg name "$SNIPPET_NAME" \
    '.[$name].body | type == "array"' \
    "$SNIPPET_FILE" >/dev/null; then

    echo "Error: snippet not found or body is invalid: $SNIPPET_NAME" >&2
    exit 1
fi

if [ -e "$COMPONENT_DIR" ]; then
    echo "Error: component already exists: $COMPONENT_DIR" >&2
    exit 1
fi

CURRENT_DATE="$(date +%d)"
CURRENT_MONTH="$(LC_ALL=C date +%b)"
CURRENT_YEAR="$(date +%Y)"

mkdir -p "$TESTS_DIR"

jq -r \
    --arg name "$SNIPPET_NAME" \
    --arg filename "$FILE_NAME.h" \
    --arg filename_upper "$HEADER_GUARD" \
    --arg current_date "$CURRENT_DATE" \
    --arg current_month "$CURRENT_MONTH" \
    --arg current_year "$CURRENT_YEAR" \
    '
    .[$name].body[]
    | gsub("\\$\\{TM_FILENAME\\}"; $filename)
    | gsub("\\$\\{CURRENT_DATE\\}"; $current_date)
    | gsub(
        "\\$\\{CURRENT_MONTH_NAME_SHORT\\}";
        $current_month
      )
    | gsub("\\$\\{CURRENT_YEAR\\}"; $current_year)
    | gsub(
        "\\$\\{TM_FILENAME_BASE/\\(\\.\\*\\)/\\$\\{1:/upcase\\}/\\}";
        $filename_upper
      )
    | gsub(
        "\\$\\{[0-9]+:(?<default>[^}]*)\\}";
        "\(.default)"
      )
    | gsub("\\$0"; "")
    ' \
    "$SNIPPET_FILE" > "$HEADER_FILE"

cat > "$SOURCE_FILE" <<EOF
#include "$FILE_NAME.h"
EOF

cat > "$TEST_FILE" <<EOF
#include "unity.h"

#include "$FILE_NAME.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_${FILE_NAME}_placeholder(void)
{
    TEST_PASS();
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_${FILE_NAME}_placeholder);

    return UNITY_END();
}
EOF

cat > "$CMAKE_FILE" <<EOF
add_library($COMPONENT_TARGET STATIC
    $FILE_NAME.c
)

target_include_directories($COMPONENT_TARGET PUBLIC
    \${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features($COMPONENT_TARGET PUBLIC
    c_std_11
)

target_link_libraries(project_components INTERFACE
    $COMPONENT_TARGET
)

if(BUILD_TESTING)
    add_executable($TEST_TARGET
        Tests/test_$FILE_NAME.c
    )

    target_link_libraries($TEST_TARGET PRIVATE
        $COMPONENT_TARGET
        unity
    )

    target_compile_options($TEST_TARGET PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    add_test(
        NAME $FILE_NAME
        COMMAND $TEST_TARGET
    )
endif()
EOF

echo "Component created:"
echo "  Components/$COMPONENT_NAME"
echo
echo "Files:"
echo "  Components/$COMPONENT_NAME/$FILE_NAME.h"
echo "  Components/$COMPONENT_NAME/$FILE_NAME.c"
echo "  Components/$COMPONENT_NAME/Tests/test_$FILE_NAME.c"
echo "  Components/$COMPONENT_NAME/CMakeLists.txt"
