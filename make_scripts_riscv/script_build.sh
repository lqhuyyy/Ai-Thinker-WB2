#!/bin/bash
# script_build.sh - Build firmware, generate OTA image, add MD5 header (no flash to MCU)
# Được gọi từ make ota trong project.mk
# Biến truyền vào: PROJECT_PATH, BL60X_SDK_PATH

PROJECT_PATH="${PROJECT_PATH:?PROJECT_PATH is not set}"
BL60X_SDK_PATH="${BL60X_SDK_PATH:?BL60X_SDK_PATH is not set}"

cd "$PROJECT_PATH"

LOGFILE=$(mktemp /tmp/build_flash_XXXXXX.log)
TAIL_PID=""
MAKE_PID=""

# Kill process và tất cả con cháu của nó
kill_tree() {
    local pid=$1
    local children
    children=$(pgrep -P "$pid" 2>/dev/null) || true
    for child in $children; do
        kill_tree "$child"
    done
    kill -9 "$pid" 2>/dev/null || true
}

# Kill toàn bộ pipeline tail (tail + grep)
kill_tail_pipeline() {
    if [ -n "$TAIL_PID" ]; then
        # Kill process group của tail pipeline
        kill -- -"$TAIL_PID" 2>/dev/null || true
        # Fallback: kill từng thằng
        kill "$TAIL_PID" 2>/dev/null || true
        pkill -P "$TAIL_PID" 2>/dev/null || true
        wait "$TAIL_PID" 2>/dev/null || true
    fi
}

# Dọn dẹp khi Ctrl+C hoặc thoát
cleanup() {
    echo ""
    echo -e "\033[31m=== Interrupted! Cleaning up... ===\033[0m"
    kill_tail_pipeline
    kill_tree "$MAKE_PID" 2>/dev/null || true
    rm -f "$LOGFILE"
    exit 1
}
trap cleanup INT
trap "rm -f '$LOGFILE'" EXIT

OTA_DIR="$PROJECT_PATH/build_out/ota"
OTA_FILE="$OTA_DIR/FW_OTA.bin.xz"
OTA_CONFIG="$BL60X_SDK_PATH/make_scripts_riscv/ota_config.py"
WHOLE_FLASH_SOURCE="$BL60X_SDK_PATH/tools/flash_tool/chips/bl602/img_create_iot/whole_flash_data.bin"
WHOLE_FLASH_DEST="$OTA_DIR/whole_flash_data.bin"
WHOLE_FLASH_TIMEOUT_SECONDS="${WHOLE_FLASH_TIMEOUT_SECONDS:-120}"

echo -e "\033[36m=== Starting build & OTA generation ===\033[0m"

# Xoá file OTA cũ để kiểm tra chính xác file mới được tạo
if [ -f "$OTA_FILE" ]; then
    echo -e "\033[33m=== Removing old OTA file ===\033[0m"
    rm -f "$OTA_FILE"
fi

# Xoá whole-flash image cũ ở cả nguồn và thư mục OTA.
rm -f "$WHOLE_FLASH_SOURCE" "$WHOLE_FLASH_DEST"
if [ -e "$WHOLE_FLASH_SOURCE" ] || [ -e "$WHOLE_FLASH_DEST" ]; then
    echo -e "\033[31m=== Failed to remove stale whole_flash_data.bin ===\033[0m" >&2
    exit 1
fi

whole_flash_ready() {
    local size_before
    local size_after

    [ -f "$WHOLE_FLASH_SOURCE" ] || return 1
    size_before=$(stat -c '%s' "$WHOLE_FLASH_SOURCE") || return 1
    [ "$size_before" -gt 0 ] || return 1
    sleep 1
    size_after=$(stat -c '%s' "$WHOLE_FLASH_SOURCE") || return 1
    [ "$size_before" -eq "$size_after" ]
}

wait_for_whole_flash() {
    local elapsed=0

    while [ "$elapsed" -lt "$WHOLE_FLASH_TIMEOUT_SECONDS" ]; do
        if whole_flash_ready; then
            return 0
        fi
        sleep 1
        elapsed=$((elapsed + 1))
    done

    echo -e "\033[31m=== whole_flash_data.bin was not generated within ${WHOLE_FLASH_TIMEOUT_SECONDS}s ===\033[0m" >&2
    echo "Expected source: $WHOLE_FLASH_SOURCE" >&2
    return 1
}

copy_whole_flash() {
    wait_for_whole_flash || return 1
    cp -f "$WHOLE_FLASH_SOURCE" "$WHOLE_FLASH_DEST" || return 1
    cmp "$WHOLE_FLASH_SOURCE" "$WHOLE_FLASH_DEST" || return 1
}

# PYTHONUNBUFFERED=1: buộc flash tool (Python) flush output ngay, không buffer
# make -j$(nproc): build song song tất cả CPU cores
# make flash: chạy flash tool tạo OTA
(PYTHONUNBUFFERED=1 make -j$(nproc) && PYTHONUNBUFFERED=1 make flash) >"$LOGFILE" 2>&1 &
MAKE_PID=$!

# Hiển thị log real-time (lọc bỏ command-line warnings)
# --line-buffered: grep xuất từng dòng ngay, không buffer
tail -f "$LOGFILE" 2>/dev/null | grep --line-buffered -vE "^<command-line>:" &
TAIL_PID=$!

# Theo dõi output, khi thấy "Please Press Reset Key!" hoặc lỗi serial port hoặc OTA file xong
FOUND=false
WHOLE_FLASH_WAIT_STARTED=""
WHOLE_FLASH_TIMED_OUT=false
while kill -0 "$MAKE_PID" 2>/dev/null; do
    # Bắt đầu timeout khi flash tool đã tạo OTA hoặc chuyển sang bước chờ nạp.
    if [ -z "$WHOLE_FLASH_WAIT_STARTED" ] && \
       { [ -f "$OTA_FILE" ] || grep -qE "Please Press Reset Key!|could not open port" "$LOGFILE" 2>/dev/null; }; then
        WHOLE_FLASH_WAIT_STARTED=$SECONDS
    fi
    if [ -n "$WHOLE_FLASH_WAIT_STARTED" ] && \
       [ $((SECONDS - WHOLE_FLASH_WAIT_STARTED)) -ge "$WHOLE_FLASH_TIMEOUT_SECONDS" ]; then
        WHOLE_FLASH_TIMED_OUT=true
        kill_tail_pipeline
        kill_tree "$MAKE_PID"
        wait "$MAKE_PID" 2>/dev/null || true
        break
    fi

    # Check 1: Flash tool đợi reset key → các image đã tạo xong
    if grep -q "Please Press Reset Key!" "$LOGFILE" 2>/dev/null && \
       [ -f "$OTA_FILE" ] && whole_flash_ready; then
        FOUND=true
        kill_tail_pipeline
        echo ""
        echo -e "\033[33m=== OTA generated. Stopping flash tool (not flashing to MCU)... ===\033[0m"
        kill_tree "$MAKE_PID"
        wait "$MAKE_PID" 2>/dev/null || true
        break
    fi
    # Check 2: Serial port không có nhưng các image đã tạo xong.
    if grep -q "could not open port" "$LOGFILE" 2>/dev/null && \
       [ -f "$OTA_FILE" ] && whole_flash_ready; then
        FOUND=true
        kill_tail_pipeline
        echo ""
        echo -e "\033[33m=== Serial port not found, but OTA images are ready. Stopping... ===\033[0m"
        kill_tree "$MAKE_PID"
        wait "$MAKE_PID" 2>/dev/null || true
        break
    fi
    # Check 3: Cả OTA và whole-flash image đã xuất hiện và ghi ổn định.
    if [ -f "$OTA_FILE" ] && whole_flash_ready; then
        FOUND=true
        kill_tail_pipeline
        echo ""
        echo -e "\033[33m=== OTA images detected. Stopping flash tool (not flashing to MCU)... ===\033[0m"
        kill_tree "$MAKE_PID"
        wait "$MAKE_PID" 2>/dev/null || true
        break
    fi
    sleep 0.2
done

if $WHOLE_FLASH_TIMED_OUT; then
    echo -e "\033[31m=== whole_flash_data.bin was not generated within ${WHOLE_FLASH_TIMEOUT_SECONDS}s ===\033[0m" >&2
    echo "Expected source: $WHOLE_FLASH_SOURCE" >&2
    exit 1
fi

if ! $FOUND; then
    kill_tail_pipeline
    echo -e "\033[31m=== make flash ended before OTA generation ===\033[0m"
    cat "$LOGFILE"
    exit 1
fi

# Đợi flash tool ghi xong whole-flash image rồi copy vào thư mục OTA.
echo ""
echo -e "\033[36m=== Copying whole flash image ===\033[0m"
if ! copy_whole_flash; then
    echo -e "\033[31m=== Failed to copy whole_flash_data.bin ===\033[0m" >&2
    echo "Source:      $WHOLE_FLASH_SOURCE" >&2
    echo "Destination: $WHOLE_FLASH_DEST" >&2
    exit 1
fi

# Hiển thị file OTA
echo ""
echo -e "\033[36m=== OTA files (before MD5 header) ===\033[0m"
ls -la "$OTA_DIR/" 2>/dev/null || { echo "No OTA directory found"; exit 1; }

# Thêm MD5 header vào file OTA bằng ota_config.py
echo ""
echo -e "\033[36m=== Adding MD5 header to OTA file ===\033[0m"
if [ -f "$OTA_FILE" ]; then
    python3 "$OTA_CONFIG" "$OTA_FILE" -c TG -o "$OTA_DIR/result.bin"
    echo ""
    echo -e "\033[36m=== OTA files (after MD5 header) ===\033[0m"
    ls -la "$OTA_DIR/"
else
    echo -e "\033[31m=== FW_OTA.bin.xz not found! ===\033[0m"
    exit 1
fi

if [ ! -s "$WHOLE_FLASH_DEST" ]; then
    echo -e "\033[31m=== whole_flash_data.bin copy is missing or empty ===\033[0m" >&2
    exit 1
fi
printf 'Whole flash: %s (%s bytes)\n' \
    "$WHOLE_FLASH_DEST" "$(stat -c '%s' "$WHOLE_FLASH_DEST")"

echo ""
echo -e "\033[32m=== Build complete (no flash) ===\033[0m"
