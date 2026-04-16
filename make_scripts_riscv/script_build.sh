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

echo -e "\033[36m=== Starting build & OTA generation ===\033[0m"

# Xoá file OTA cũ (nếu có) để kiểm tra chính xác file mới được tạo
if [ -f "$OTA_FILE" ]; then
    echo -e "\033[33m=== Removing old OTA file ===\033[0m"
    rm -f "$OTA_FILE"
fi

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
while kill -0 "$MAKE_PID" 2>/dev/null; do
    # Check 1: Flash tool đợi reset key → OTA đã tạo xong
    if grep -q "Please Press Reset Key!" "$LOGFILE" 2>/dev/null; then
        FOUND=true
        kill_tail_pipeline
        echo ""
        echo -e "\033[33m=== OTA generated. Stopping flash tool (not flashing to MCU)... ===\033[0m"
        kill_tree "$MAKE_PID"
        wait "$MAKE_PID" 2>/dev/null || true
        break
    fi
    # Check 2: Serial port không có → OTA vẫn đã tạo xong
    if grep -q "could not open port" "$LOGFILE" 2>/dev/null; then
        if [ -f "$OTA_FILE" ]; then
            FOUND=true
            kill_tail_pipeline
            echo ""
            echo -e "\033[33m=== Serial port not found, but OTA file already generated. Stopping... ===\033[0m"
            kill_tree "$MAKE_PID"
            wait "$MAKE_PID" 2>/dev/null || true
            break
        fi
    fi
    # Check 3: File OTA đã xuất hiện → có thể flash tool chưa kịp in "Please Press Reset Key!"
    if [ -f "$OTA_FILE" ]; then
        # Đợi thêm 2 giây để flash tool hoàn tất ghi file
        sleep 2
        if [ -f "$OTA_FILE" ]; then
            FOUND=true
            kill_tail_pipeline
            echo ""
            echo -e "\033[33m=== OTA file detected. Stopping flash tool (not flashing to MCU)... ===\033[0m"
            kill_tree "$MAKE_PID"
            wait "$MAKE_PID" 2>/dev/null || true
            break
        fi
    fi
    sleep 0.2
done

if ! $FOUND; then
    kill_tail_pipeline
    echo -e "\033[31m=== make flash ended before OTA generation ===\033[0m"
    cat "$LOGFILE"
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

echo ""
echo -e "\033[32m=== Build complete (no flash) ===\033[0m"
