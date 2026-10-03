# HƯỚNG DẪN CẬP NHẬT CODE QUA OTA (GITHUB PUBLIC)

Repo: https://github.com/letin1712-collab/chicken_alarm (phải để **Public**)

## Cơ chế
1. Mạch đọc `version.txt` trên GitHub.
2. Nếu số phiên bản khác `FIRMWARE_VERSION` trong `main.cpp` -> tải `firmware.bin` từ Releases mới nhất.
3. Nạp xong, mạch tự khởi động lại.

## Thiết lập một lần duy nhất
1. GitHub -> repo -> Settings -> General -> kéo xuống **Danger Zone** -> Change visibility -> **Public**.
2. **Thu hồi token cũ** (đã bị lộ): Settings -> Developer settings -> Personal access tokens -> Revoke.
3. Nạp bản code này vào mạch bằng cáp USB (mạch cần có sẵn code OTA mới cập nhật từ xa được).
4. Đảm bảo `version.txt` trên GitHub trùng `FIRMWARE_VERSION` (hiện là `1.0.1`).

## Mỗi lần muốn thay đổi code

### Bước 1: Sửa code + tăng phiên bản
Sửa code trong `src/`, rồi tăng số phiên bản trong `src/main.cpp`:
```cpp
#define FIRMWARE_VERSION "1.0.2"
```
Quên bước này thì mạch sẽ không nhận ra bản mới.

### Bước 2: Build
```bash
pio run
```
Hoặc bấm dấu ✓ trong VSCode. Kết quả: `.pio/build/esp32dev/firmware.bin`

### Bước 3: Tạo Release và đính kèm firmware.bin
Cách A - giao diện web: Releases -> Draft a new release -> tag mới `v1.0.2` -> kéo file `firmware.bin` vào -> Publish release.

Cách B - lệnh (cần GitHub CLI `gh`):
```bash
gh release create v1.0.2 .pio/build/esp32dev/firmware.bin --title v1.0.2
```
Tên file phải đúng là `firmware.bin`, và mỗi lần dùng một tag mới.

### Bước 4: Sửa version.txt (làm SAU bước 3)
Nội dung file chỉ một dòng, ví dụ `1.0.2`, rồi commit và push:
```bash
echo 1.0.2 > version.txt
git add version.txt src
git commit -m "v1.0.2"
git push
```

### Bước 5: Kích hoạt trên mạch
1. Mạch phải đang nối Wi-Fi nhà **có Internet** (không chỉ kết nối vào AP `chicken-alarm`).
2. Giao diện web không còn nút OTA. Kích hoạt bằng lệnh sau (thay `IP_MACH` bằng IP của mạch, máy tính phải cùng mạng Wi-Fi):
   ```bash
   curl -X POST http://IP_MACH/api/ota
   ```
3. Theo dõi Serial Monitor (115200). Đợi khoảng 30-60 giây, **không tắt nguồn**. Trang web sẽ đứng trong lúc tải, đó là bình thường.
4. Mạch tự khởi động lại và chạy bản mới.

## Xử lý lỗi thường gặp
| Hiện tượng trên Serial | Cách xử lý |
|---|---|
| `Lỗi khi lấy phiên bản (HTTP 404)` | Sai URL `version.txt` hoặc repo chưa Public |
| `Đang dùng phiên bản mới nhất` | `version.txt` trùng `FIRMWARE_VERSION`; chưa sửa version.txt hoặc đợi vài phút do cache |
| `Cập nhật thất bại ... HTTP 404` | Release chưa có file đúng tên `firmware.bin` |
| `WiFi chưa kết nối` | Vào mục Kết nối Wi-Fi để lưu Wi-Fi nhà |
| Mạch cứ cập nhật lặp lại | `FIRMWARE_VERSION` trong bin chưa khớp `version.txt`; kiểm tra lại Bước 1 và 4 |

## Lưu ý bảo mật
- Không dán token GitHub vào code (`GITHUB_TOKEN` để `""`).
- Repo Public nên mật khẩu AP (`AP_PASSWORD` trong `main.cpp`) ai cũng đọc được. Nên đổi nếu cần.