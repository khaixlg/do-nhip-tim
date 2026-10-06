# 💓 ESP32 Heart Rate Monitor

Thiết bị đo nhịp tim (BPM) dùng ESP32s3 + cảm biến MAX30102**, hiển thị đồ thị tín hiệu trên **màn hình OLED** và **giao diện web realtime**. Có cảnh báo bằng **LED + còi** khi nhịp tim cao, và hỗ trợ **chế độ Offline** (không cần WiFi).

 ⚠️ Lưu ý: Đây là project học tập không phải thiết bị y tế. Không dùng kết quả để chẩn đoán hay thay thế tư vấn của bác sĩ.

---

 ✨ Tính năng

- Đo nhịp tim bằng tín hiệu hồng ngoại (IR) từ MAX30102, lấy trung bình 8 lần đập gần nhất để số liệu ổn định.
- OLED SSD1306 (128x64): vẽ đồ thị tín hiệu IR, hiển thị BPM, đếm ngược 3-2-1 khi bắt đầu đo.
- Web dashboard: (ESP32 làm web server, cập nhật bằng **Server-Sent Events** mỗi 80 ms):
  + Hiển thị BPM hiện tại và đồ thị dạng ECG
  + Phân loại trạng thái: *Nhịp chậm* (< 60), *Bình thường* (60–100), *Nhịp nhanh* (> 100)
  + Thống kê MAX / MIN / AVG của tối đa 12 mẫu (mỗi 5 giây lấy 1 mẫu, tương ứng 1 phút đo)
  + Nút **Bắt đầu / Dừng đo** trên web
- **Nút nhấn vật lý** (GPIO15) để Start/Stop, có chống dội phím (debounce).
- **Cảnh báo nhịp tim cao** bằng LED + còi nhấp nháy, có *hysteresis* để tránh nhấp nháy liên tục quanh ngưỡng:
  - Bật khi BPM > **100**
  - Tắt khi BPM ≤ **90** và đã cảnh báo tối thiểu **3 giây**
- **Chế độ Offline:** giữ nút vật lý khi khởi động (≥ 2 giây) để bỏ qua WiFi. Nếu không kết nối được WiFi trong 8 giây, thiết bị cũng tự chuyển sang Offline và vẫn hoạt động bình thường với OLED + nút + còi/đèn.
- Tự phát hiện lỗi cảm biến/màn hình: OLED lỗi thì LED nhấp nháy, MAX30102 lỗi thì còi kêu và hiện thông báo lên OLED.

---

## 🧰 Phần cứng

| Linh kiện | Ghi chú |
|---|---|
| ESP32 | Bo mạch có hỗ trợ GPIO 4, 8, 9, 15, 18 |
| Cảm biến nhịp tim **MAX30102** | I2C, địa chỉ `0x57` |
| OLED **SSD1306** 128x64 | I2C, địa chỉ `0x3C` |
| LED | Cảnh báo (kèm điện trở hạn dòng) |
| Còi (buzzer) | Loại **active** (điều khiển bằng HIGH/LOW) |
| Nút nhấn | Kéo xuống GND, dùng `INPUT_PULLUP` |

---
### Sơ đồ đấu nối

| Thiết bị | Chân ESP32 |
|---|---|
| MAX30102 + OLED SDA | **GPIO 8** |
| MAX30102 + OLED SCL | **GPIO 9** |
| LED cảnh báo | **GPIO 4** |
| Còi (Buzzer) | **GPIO 18** |
| Nút nhấn (về GND) | **GPIO 15** |

MAX30102 và OLED dùng chung bus I2C (cùng SDA/SCL, cấp nguồn 5v và GND chung).
<img width="974" height="769" alt="image" src="https://github.com/user-attachments/assets/07b10f1c-3423-46f7-b6a6-3d4ac24a0887" />

---

## 📦 Thư viện cần cài (Arduino IDE)

- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `Arduino_JSON`
- `SparkFun MAX3010x Pulse and Proximity Sensor Library` (cung cấp `MAX30105.h` và `heartRate.h`)
- AsyncTCP by ESP32Async
- ESPAsyncWebServer by ESP32Async
---

## 🚀 Cài đặt & nạp code

1. Clone repo:
   ```bash
   git clone https://github.com/<ten-ban>/<ten-repo>.git
   ```
2. Mở `heart.ino` bằng Arduino IDE (giữ `web_server.h` **cùng thư mục** với `heart.ino`).
3. Sửa thông tin WiFi trong `heart.ino`:
   const char* ssid = "TEN_WIFI";
   const char* password = "MAT_KHAU_WIFI";
4. Chọn đúng board ESP32 và cổng COM, rồi **Upload**.
5. Mở **Serial Monitor** (baud `115200`) để xem log và địa chỉ IP.

---

## 📖 Cách sử dụng

### Chế độ Online (có WiFi)

1. Cấp nguồn cho ESP32, đợi OLED hiện địa chỉ IP (ví dụ `192.168.1.xx`).
2. Điện thoại/máy tính **cùng mạng WiFi**, mở trình duyệt và truy cập địa chỉ IP đó.
3. Đặt ngón tay lên cảm biến MAX30102 (giữ yên, áp nhẹ).
4. Bấm **Bắt đầu đo** trên web hoặc nhấn nút vật lý GPIO15.
5. Xem BPM, đồ thị và thống kê MAX/MIN/AVG. Bấm **Dừng đo** để kết thúc.

> Trang web tải Bootstrap và Google Fonts từ CDN, nên thiết bị mở web cần có Internet để hiển thị đẹp nhất. Chức năng đo vẫn hoạt động nếu CDN không tải được, chỉ giao diện bị đơn giản hơn.

### Chế độ Offline (không cần WiFi)

- **Cách 1:** Giữ nút GPIO15 khi cấp nguồn cho đến khi OLED báo `CHE DO: OFFLINE`.
- **Cách 2:** Nếu WiFi không kết nối được sau 8 giây, thiết bị tự chuyển sang Offline.
- Khi đó dùng **nút vật lý** để Start/Stop, xem BPM trên OLED. Không có web server.

### Các trạng thái hiển thị OLED

| Màn hình | Ý nghĩa |
|---|---|
| Logo khởi động | Hiển thị 3 giây khi bật máy |
| `Connecting WiFi...` | Đang kết nối WiFi |
| `Nhan START de do` | Sẵn sàng, chờ bắt đầu đo |
| Đếm ngược 3-2-1 | Bắt đầu đo |
| Đồ thị + `♥ : xx BPM` | Đang đo |
| `STOP` | Đã dừng đo |
| `MAX30102 ERROR` | Không tìm thấy cảm biến, kiểm tra dây SDA/SCL |

---

## ⚙️ Tham số cấu hình

Các hằng số có thể chỉnh trong `heart.ino`:

| Tham số | Mặc định | Ý nghĩa |
|---|---|---|
| `HIGH_HR_THRESHOLD` | `100` | Ngưỡng BẬT cảnh báo (BPM) |
| `HIGH_HR_RESET` | `90` | Ngưỡng TẮT cảnh báo (BPM) |
| `ALARM_MIN_HOLD_MS` | `3000` | Thời gian cảnh báo tối thiểu trước khi được tắt |
| `ALARM_BEEP_PERIOD_MS` | `250` | Chu kỳ nhấp nháy còi/đèn |
| `WIFI_CONNECT_TIMEOUT_MS` | `8000` | Thời gian tối đa chờ kết nối WiFi |
| `OFFLINE_BOOT_HOLD_MS` | `2000` | Thời gian giữ nút lúc khởi động để vào Offline |
| `SSE_INTERVAL` | `80` | Chu kỳ gửi dữ liệu lên web (ms) |
| `rateSize` | `8` | Số lần đập dùng để lấy trung bình BPM |

Cấu hình cảm biến MAX30102 (trong `setup()`): LED brightness `0x1F`, sample average `4`, LED mode `2` (Red + IR), sample rate `100 Hz`, pulse width `411`, ADC range `4096`.

---

## 🔬 Nguyên lý hoạt động

1. MAX30102 đọc giá trị hồng ngoại (IR). Nếu IR < 35000 thì coi như **chưa đặt ngón tay** và BPM = 0.
2. Thuật toán `checkForBeat()` (thư viện SparkFun) phát hiện mỗi nhịp đập. Khoảng thời gian giữa 2 nhịp được đổi thành BPM tức thời (chỉ nhận trong khoảng 30–255 BPM).
3. BPM hiển thị là **trung bình của 8 lần đo gần nhất**.
4. ESP32 gửi JSON qua SSE đến trình duyệt:
   ```json
   {
     "heartbeat_Signal": 85000,
     "BPM_TimeStamp": "00:00:12",
     "BPM_Val": 72,
     "BPM_State": true
   }
   ```
5. Điều khiển từ web: trình duyệt gọi `GET /BTN_Comd?BTN_Start_Get_BPM=START` hoặc `STOP`.

> 📝 Đồ thị ECG trên web là **dạng sóng mô phỏng** vẽ theo giá trị BPM nhận được (không phải tín hiệu ECG thật). Đồ thị tín hiệu IR thật hiển thị trên OLED.
---

## 🛠️ Xử lý sự cố

| Hiện tượng | Cách khắc phục |
|---|---|
| LED nhấp nháy chậm, không lên OLED | Không tìm thấy OLED: kiểm tra dây SDA/SCL và địa chỉ `0x3C` |
| Còi kêu `bíp` mỗi giây, OLED báo `MAX30102 ERROR` | Không tìm thấy MAX30102 ở `0x57`: kiểm tra dây và nguồn 3.3V |
| BPM luôn là 0 | Chưa đặt ngón tay đúng, hoặc IR < 35000. Áp ngón tay nhẹ, giữ yên |
| Không vào được web | Kiểm tra cùng mạng WiFi, xem IP trên OLED/Serial; có thể đã vào Offline do WiFi lỗi |
| BPM nhảy loạn | Giữ tay yên, tránh ánh sáng mạnh chiếu trực tiếp vào cảm biến |

---
