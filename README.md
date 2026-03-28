# 🛡️ Embedded Network Intrusion Detection System (ENIDS)
**嵌入式網路入侵偵測系統 (STM32H7)**

An advanced network security monitor based on **STM32H753ZI**, featuring **Real-time scanning**, **OUI Vendor Identification**, **FreeRTOS multi-tasking**, and **Telegram Cloud Alerting**.  
本系統使用 **STM32H753ZI** 搭配 **LAN8742 PHY**，結合 **FreeRTOS** 與 **lwIP** 協議棧，實作具備廠商識別與雲端告警功能的即時網路監控系統。

---

## 📌 Features / 功能介紹

| Feature 功能 | Description 說明 |
|--------------|------------------|
| 🚀 High-Speed Scan | 利用 Cortex-M7 480MHz 效能，毫秒級掃描全網段 (1~254) |
| 🔍 OUI Identification | 透過 MAC 位址前三碼自動識別設備廠商 (如 Apple, ASUS) |
| 🛡️ Smart Whitelist | 系統啟動初期自動建立合法白名單，實現智慧監控 |
| 🚨 Intruder Alert | 發現陌生 MAC 即判定為 `[INTRUDER!]` 並觸發警報 |
| ☁️ Cloud Notification | 整合 **Flask API**，將入侵資訊即時推播至 **Telegram** |
| 🧵 Multi-tasking | 使用 **FreeRTOS** 確保掃描與通訊任務並行、互不干擾 |

---

## 🧑‍💻 Author / 作者  
**Wu Fu-Min (吳富民)** Department of Computer Science and Information Engineering  
Kun Shan University / 崑山科技大學資工系

---

## 🧱 Hardware Components / 硬體元件

| Component 元件 | Qty 數量 | Description 說明 |
|----------------|---------|------------------|
| STM32H753ZI (NUCLEO) | 1 | 主控板 (Arm Cortex-M7, 480MHz) |
| LAN8742 PHY | 1 | 乙太網路實體層晶片 (Board Integrated) |
| RJ45 Cable | 1 | 實體網路連線 |
| Flask Server | 1 | 部署於伺服器端作為告警網關 (Python) |

---

## 🔌 System Configuration / 系統配置

### 🛠️ Software Stack / 軟體技術

| Layer 層級 | Technology 採用技術 |
|------------|-------------------|
| **OS** | FreeRTOS v10.3.1 (Multi-tasking Management) |
| **Network** | lwIP v2.1.2 (TCP/IP Stack, ARP/ICMP Support) |
| **Backend** | Python Flask (Alert Gateway) |
| **Platform** | Telegram Bot API (Instant Mobile Alert) |

---

## 🧠 System Logic / 系統流程

1. **Initialization (初始化)**：啟動時鐘、ETH 介面，並掛載 FreeRTOS 與 lwIP 任務。
2. **Learning Mode (學習模式)**：前 1~3 次掃描為建立期，自動紀錄已知合法設備。
3. **Monitoring Mode (監控模式)**：比對在線設備與白名單，若不匹配則判定為 `[INTRUDER!]`。
4. **Cloud Alerting (雲端告警)**：STM32 發送 HTTP 請求至 Flask，並觸發 Telegram API 推播。

---

## 📂 資料夾結構 (Directory Structure)

- 📁 `firmware/`：存放 STM32CubeIDE 專案程式碼與 LwIP 配置。
- 📁 `server/`：存放 Flask 後端 (Python) 腳本 `arp.py`。
- 📁 `docs/`：存放專案 PDF 說明文件與報告。

---

## 🚀 Future Improvements / 未來可改進方向

| Feature | Description 說明 |
|---------|------------------|
| 💾 Flash Storage | 將第一次確認的白名單存入 Flash，重啟後自動載入 |
| 🎮 Interactive Bot | 透過 Telegram 按鈕提供 [信任此設備] 或 [封鎖] 功能 |
| 📊 Dashboard | 利用 lwIP 內建 HTTPD 服務開發 Web 視覺化儀表板 |

---

## 📄 License / 授權
This project is for academic and personal learning use.  
本專案開放作為學習用途，禁止未授權商業使用。
