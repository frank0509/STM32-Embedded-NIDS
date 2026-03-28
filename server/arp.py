from flask import Flask, request
import requests

app = Flask(__name__)

# 配置你的 Telegram Bot 資訊
TG_TOKEN = ""
TG_CHAT_ID = ""

@app.route('/get', methods=['GET'])
def alert():
    # 接收來自 STM32 的參數
    ip_last = request.args.get('ip')
    mac = request.args.get('mac')
    vendor = request.args.get('vendor', 'Unknown')

    msg = f"⚠️ 偵測到入侵者！\nIP: 192.168.1.{ip_last}\nMAC: {mac}\n廠商: {vendor}"

    # 轉發 Telegram
    url = f"https://api.telegram.org/bot{TG_TOKEN}/sendMessage"
    try:
        requests.post(url, json={"chat_id": TG_CHAT_ID, "text": msg})
        print(f"[OK] 警報已傳送: IP .{ip_last}")
        return "OK", 200
    except Exception as e:
        print(f"[ERR] {e}")
        return "Error", 500

if __name__ == '__main__':
    # 這裡必須是 0.0.0.0 且 port 5001
    app.run(host='0.0.0.0', port=5001)
