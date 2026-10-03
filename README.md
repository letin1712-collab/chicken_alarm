# Báo thức ESP32 + DFPlayer Mini

## Nối dây

| ESP32 | DFPlayer Mini | Ghi chú |
|---|---|---|
| GPIO17 / TX2 | RX | Nên mắc điện trở 1 kΩ nối tiếp |
| GPIO16 / RX2 | TX | UART 9600 baud |
| GND | GND | Chung mass với PAM8403 |
| 5V ổn định | VCC | Nguồn đủ dòng cho DFPlayer và ampli |

| DFPlayer Mini | PAM8403 | Ghi chú |
|---|---|---|
| DAC_L | L-IN | Tín hiệu âm thanh trái |
| DAC_R | R-IN | Tín hiệu âm thanh phải |
| GND | GND | Chung mass |

Loa nối vào cặp `L+ / L-` hoặc `R+ / R-` của PAM8403. Không nối cực loa âm về GND; đầu ra PAM8403 là BTL. Không nối chân `SPK1/SPK2` của DFPlayer vào đầu vào PAM8403. Nên cấp nguồn 5 V riêng, đủ dòng cho PAM8403; nối chung GND nguồn với ESP32. Tránh cấp ampli từ chân 3V3 của ESP32.

```text
ESP32 GPIO17 (TX2) --[1k]--> RX   DFPlayer   DAC_L --> L-IN  PAM8403 --> Loa trái
ESP32 GPIO16 (RX2) <-------- TX              DAC_R --> R-IN             --> Loa phải
ESP32 GND ------------------ GND ------------ GND
5V ổn định ---------------- VCC             5V PAM8403 (nguồn đủ dòng)
```

## Nạp và sử dụng

1. Nạp chương trình cho ESP32 và mở Serial Monitor ở 115200 baud để xem trạng thái khởi động.
2. ESP32 phát mạng Wi-Fi `chicken-alarm` (mật khẩu hiện đặt trong `src/main.cpp`). Kết nối điện thoại vào mạng này rồi mở `http://192.168.4.1`.
3. Nhập tên và mật khẩu Wi-Fi nhà có Internet trong mục **Kết nối Wi-Fi**, sau đó chọn **Lưu Wi-Fi**. Chờ ESP32 kết nối; điện thoại có thể cần chuyển lại sang mạng Wi-Fi nhà.
4. Mở địa chỉ IP ESP32 hiển thị ở mục trạng thái trên điện thoại đang dùng cùng mạng Wi-Fi. Nếu vẫn kết nối AP của ESP32, dùng `http://192.168.4.1`.

## Hướng dẫn sử dụng web

### Cài báo thức

1. Mở trang web của ESP32 bằng trình duyệt trên điện thoại.
2. Chọn **Giờ** và **Phút** theo giờ địa phương Việt Nam.
3. Chọn **Thời lượng** từ 1 đến 7200 giây (tối đa 2 giờ) và **Âm lượng** từ 0 đến 30. Mỗi lần chuông bắt đầu, ESP32 chọn ngẫu nhiên một bài; khi hết bài, nó chọn bài ngẫu nhiên tiếp theo cho đến hết thời lượng.
4. Nhấn **Thêm báo thức**. Có thể tạo tối đa 10 báo thức hằng ngày; mỗi mục có công tắc bật/tắt, nút sửa và nút xóa.

Thiết lập được lưu trong ESP32 nên không cần mở điện thoại hoặc giữ trang web hoạt động. Báo thức lặp lại mỗi ngày. ESP32 cần kết nối Wi-Fi có Internet để đồng bộ giờ NTP sau khi khởi động; kiểm tra trạng thái giờ trên đầu trang trước khi tin cậy báo thức.

### Phát thử và dừng

- Chọn **Bài kiểm tra** rồi nhấn **Phát thử** để kiểm tra file và âm lượng. Bài kiểm tra không dùng thời lượng báo thức.
- Nhấn **Dừng phát** để dừng âm thanh đang phát, kể cả báo thức.
- Dòng trạng thái thẻ nhớ cho biết DFPlayer đã sẵn sàng hay chưa, số file được nhận diện và trạng thái phát.

### Thêm hoặc xóa file âm thanh

1. Tắt nguồn thiết bị rồi tháo thẻ microSD khỏi DFPlayer.
2. Trên máy tính, thêm hoặc xóa file trong thư mục `MP3` hoặc ngay thư mục gốc thẻ. Đặt tên liên tục `0001.mp3`, `0002.mp3`, `0003.mp3`…; tránh để thiếu số ở giữa. Firmware thử phát theo thư mục `MP3` trước, sau đó fallback sang thứ tự file ở gốc thẻ.
3. Lắp lại thẻ, cấp nguồn cho thiết bị và mở trang web.
4. Nhấn **Quét lại số bài**, sau đó tải lại trang hoặc mở lại danh sách chọn bài.

DFPlayer Mini không hỗ trợ ESP32 tải lên hoặc xóa file trên thẻ qua UART. Không tháo thẻ khi thiết bị đang bật. Các file `.wav` trong thư mục `data` của dự án không tự được chép sang thẻ; cần chuyển chúng thành MP3 rồi chép vào đúng thư mục trên thẻ.

### Kết nối Wi-Fi và truy cập lại

- **Chưa cấu hình Wi-Fi:** kết nối điện thoại với `chicken-alarm` và vào `http://192.168.4.1`.
- **Đã kết nối Wi-Fi:** xem địa chỉ IP hiện trong trạng thái trang web hoặc Serial Monitor, rồi mở IP đó trên điện thoại cùng mạng.
- **Đổi router hoặc mật khẩu Wi-Fi:** vào mục **Kết nối Wi-Fi**, nhập thông tin mới và nhấn **Lưu Wi-Fi**.
- Khi mất Internet, web và các nút phát vẫn có thể hoạt động trong mạng nội bộ, nhưng ESP32 không thể đồng bộ giờ mới. Sau khi khởi động lại, cần có Internet để lấy lại giờ chính xác.

Thẻ microSD nên định dạng FAT32. Đặt nhạc tên tuần tự `0001.mp3`, `0002.mp3`, ... trong thư mục `MP3` hoặc ngay gốc thẻ. Các file `.wav` hiện ở thư mục `data` của dự án không tự được chép vào thẻ; cần chuyển mã thành MP3 và chép vào thẻ.

## Lưu ý

- Nếu chỉ kết nối điện thoại vào AP của ESP32 mà chưa cấu hình Wi-Fi Internet, giao diện vẫn dùng được nhưng giờ NTP chưa có nên báo thức theo giờ sẽ chưa chạy.
- Đổi mật khẩu AP trong `src/main.cpp` nếu thiết bị được dùng trong môi trường có người khác truy cập.
- Nếu ESP32 không nhận DFPlayer, kiểm tra TX/RX chéo, GND chung, nguồn ổn định và thẻ SD.