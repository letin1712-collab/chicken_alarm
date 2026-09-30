# HƯỚNG DẪN SỬ DỤNG OTA BẰNG GITHUB

Tính năng OTA (Over-The-Air) cho phép bạn nạp code mới cho ESP32 từ xa thông qua mạng WiFi mà không cần cắm cáp USB.

## CƠ CHẾ HOẠT ĐỘNG
1. ESP32 sẽ đọc một file `version.txt` (chứa phiên bản mới nhất) trên Github của bạn.
2. Nếu số phiên bản trong file `version.txt` khác (lớn hơn) so với biến `FIRMWARE_VERSION` hiện đang chạy trong ESP32, nó sẽ hiểu là có bản cập nhật.
3. ESP32 tiến hành tải file `firmware.bin` mới từ phần Releases của Github.
4. Nạp code mới và tự động khởi động lại.

---

## CÁC BƯỚC ĐỂ CẬP NHẬT CODE TỪ XA (OTA)

### Bước 1: Chuẩn bị Repository trên Github
Nếu bạn chưa có, hãy tạo một Public Repository trên Github (để ESP32 có thể tải file mà không cần xác thực).
- Tạo một file tên là `version.txt` ở thư mục gốc (hoặc ở đâu tùy bạn), trong đó chỉ ghi một dòng duy nhất là phiên bản, ví dụ: `1.0.0`
- Ghi chú lại đường link **Raw** của file này. (Mở file trên Github -> Bấm nút **Raw** -> Copy URL). Nó sẽ có dạng: `https://raw.githubusercontent.com/username/repo/main/version.txt`

### Bước 2: Viết code mới và cập nhật phiên bản
Khi bạn muốn sửa tính năng hoặc thêm code mới trong file `main.cpp` (hoặc các file khác):
1. **Rất quan trọng:** Hãy tăng số phiên bản ở dòng `#define FIRMWARE_VERSION "x.x.x"` trong file `main.cpp`. Ví dụ, từ `"1.0.0"` lên `"1.0.1"`.
2. Lưu file lại.

### Bước 3: Build ra file firmware.bin
Trong VSCode (PlatformIO):
1. Bấm nút **Build** (biểu tượng dấu ✓ ở thanh trạng thái bên dưới).
2. Khi Build thành công (`[SUCCESS]`), PlatformIO sẽ tạo ra file `firmware.bin` nằm trong thư mục: 
   `.pio/build/esp32dev/firmware.bin`

### Bước 4: Upload firmware lên Github Releases
1. Lên trang repo Github của bạn, vào phần **Releases** -> Chọn **Draft a new release**.
2. Chọn/tạo một tag mới (ví dụ `v1.0.1`).
3. Kéo thả file `firmware.bin` (ở Bước 3) vào ô đính kèm (Attach binaries by dropping them here).
4. Bấm **Publish release**.
5. Chuột phải vào file `firmware.bin` vừa được đính kèm ở Release -> Chọn **Copy link address** (Sao chép địa chỉ liên kết). Nó sẽ có dạng: `https://github.com/username/repo/releases/download/v1.0.1/firmware.bin`.
*(Ghi chú: Bạn có thể sửa URL trong `main.cpp` thành `.../releases/latest/download/firmware.bin` để luôn trỏ tới bản mới nhất, khi đó ESP32 sẽ tự tải file từ bản Release gần nhất có đính kèm file tên là `firmware.bin`)*.

### Bước 5: Cập nhật file version.txt trên Github
1. Mở file `version.txt` trên Github repo của bạn (chỉnh sửa trực tiếp trên nền web).
2. Sửa nội dung thành phiên bản mới tương ứng với bước 2 (ví dụ: `1.0.1`).
3. Bấm **Commit changes**.

### Bước 6: Kích hoạt OTA trên mạch ESP32
1. Mở giao diện Web quản lý của mạch (192.168.4.1 hoặc IP mạng nhà bạn).
2. Kéo xuống phần Kết nối WiFi, bấm nút **Cập nhật OTA**.
3. ESP32 sẽ vào Github kiểm tra `version.txt`, phát hiện phiên bản `1.0.1` lớn hơn `1.0.0` hiện tại, nó sẽ tự động tải file `firmware.bin` về và cài đặt!
*(Bạn có thể mở cổng Serial Monitor để theo dõi tiến trình 0% -> 100%)*

---

## 🔒 HƯỚNG DẪN DÀNH CHO PRIVATE REPOSITORY (REPO KÍN)
Nếu repository chứa code của bạn là **Private**, ESP32 sẽ không thể tải file nếu không có quyền truy cập. Bạn cần thực hiện các bước sau:

1. **Tạo Token (PAT)**: 
   - Đăng nhập Github, vào **Settings** -> **Developer settings** -> **Personal access tokens** -> **Tokens (classic)**.
   - Chọn **Generate new token (classic)**.
   - Đặt tên tùy ý (ví dụ: `ESP32_OTA`), phần *Expiration* có thể chọn *No expiration* (không hết hạn).
   - Đánh dấu tick vào quyền **repo** (Full control of private repositories).
   - Bấm *Generate token*, sau đó **copy ngay chuỗi token đó** (bắt đầu bằng `ghp_...`).

2. **Cập nhật token vào code ESP32**:
   - Mở file `main.cpp`, tìm dòng `#define GITHUB_TOKEN ""`.
   - Dán token của bạn vào, ví dụ: `#define GITHUB_TOKEN "ghp_xxxxxxxxxxxxxxxxxxx"`.

3. **Cách Upload File đơn giản nhất cho Private Repo**:
   - Đối với Private Repo, việc tải file từ mục Releases gặp nhiều khó khăn về mặt đường dẫn và API. Cách **dễ nhất** là bạn cứ ném thẳng file `firmware.bin` vào trong code của repo (upload trực tiếp file `firmware.bin` đè lên file cũ trên nhánh `main`).
   - Lấy đường link URL định dạng Raw cho cả 2 file `version.txt` và `firmware.bin` giống hệt nhau:
     - `OTA_VERSION_URL "https://raw.githubusercontent.com/username/repo/main/version.txt"`
     - `OTA_FIRMWARE_URL "https://raw.githubusercontent.com/username/repo/main/firmware.bin"`
   - Chỉ cần có Token là ESP32 sẽ tự động tải file Raw từ Private Repo của bạn.

