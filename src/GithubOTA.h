#pragma once
#include <Arduino.h>

class GithubOTA {
public:
    // Gọi hàm này để kiểm tra và cập nhật OTA qua GitHub.
    // versionUrl: URL chứa file text ghi version mới nhất, ví dụ: "https://raw.githubusercontent.com/user/repo/main/version.txt"
    // firmwareUrl: URL trỏ đến file .bin, ví dụ: "https://github.com/user/repo/releases/latest/download/firmware.bin"
    // currentVersion: Phiên bản hiện tại của code, ví dụ: "1.0.0"
    // githubToken (tùy chọn): Personal Access Token của Github để truy cập Private Repo, ví dụ: "ghp_xxxxxxxxxxxxxxxxxxx"
    static void checkAndUpdate(const String& versionUrl, const String& firmwareUrl, const String& currentVersion, const String& githubToken = "");
};

