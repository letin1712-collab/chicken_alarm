#include "GithubOTA.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

void GithubOTA::checkAndUpdate(const String& versionUrl, const String& firmwareUrl, const String& currentVersion, const String& githubToken) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("OTA: WiFi chưa kết nối!");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure(); // GitHub dùng HTTPS nên cần SSL, setInsecure để bỏ qua kiểm tra chứng chỉ cho tiện

    HTTPClient http;
    Serial.println("OTA: Đang kiểm tra phiên bản mới từ GitHub...");
    
    // 1. Kiểm tra version
    // Thêm tham số thời gian để tránh bị cache version.txt cũ
    http.begin(client, versionUrl + "?t=" + String(millis()));
    if (githubToken.length() > 0) {
        http.addHeader("Authorization", "token " + githubToken);
    }
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String latestVersion = http.getString();
        latestVersion.trim(); // Xóa khoảng trắng, \n dư thừa
        
        Serial.printf("OTA: Version hiện tại: %s\n", currentVersion.c_str());
        Serial.printf("OTA: Version trên GitHub: %s\n", latestVersion.c_str());
        
        if (latestVersion != currentVersion && latestVersion.length() > 0) {
            Serial.println("OTA: Phát hiện phiên bản mới! Đang tiến hành cập nhật...");
            http.end(); // Đóng kết nối cũ
            
            // 2. Tải và cài đặt firmware mới
            // httpUpdate.update() có thể xử lý việc redirect của GitHub Releases
            // Cần thiết lập follow redirects
            httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            
            t_httpUpdate_return ret;
            if (githubToken.length() > 0) {
                // Thêm token vào HTTP headers khi tải file binary
                ret = httpUpdate.update(client, firmwareUrl, "", [&githubToken](HTTPClient* httpClient) {
                    httpClient->addHeader("Authorization", "token " + githubToken);
                });
            } else {
                ret = httpUpdate.update(client, firmwareUrl);
            }
            
            switch (ret) {
                case HTTP_UPDATE_FAILED:
                    Serial.printf("OTA: Cập nhật thất bại (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
                    break;
                case HTTP_UPDATE_NO_UPDATES:
                    Serial.println("OTA: Không có bản cập nhật nào (No updates)");
                    break;
                case HTTP_UPDATE_OK:
                    Serial.println("OTA: Cập nhật thành công! Thiết bị sẽ tự động khởi động lại.");
                    break;
            }
        } else {
            Serial.println("OTA: Đang dùng phiên bản mới nhất.");
        }
    } else {
        Serial.printf("OTA: Lỗi khi lấy phiên bản (HTTP %d)\n", httpCode);
    }
    
    http.end();
}