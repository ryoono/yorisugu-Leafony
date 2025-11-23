import serial
import csv
from datetime import datetime

# ==== 設定 ====
PORT = "COM5"          # ← 自分の環境のポート名に変更 (例: "COM4", "COM5"...)
BAUDRATE = 115200
CSV_FILENAME = "takenoko2(light_high).csv"
NUM_FEATURES = 18      # 3 x 6

TARGET_SAMPLES = 250   # ★目標サンプル数（表示にも使用）

def main():
    # シリアルポートを開く
    ser = serial.Serial(PORT, BAUDRATE, timeout=1)
    print(f"Opened {PORT} @ {BAUDRATE}bps")

    # CSVファイルを開く（追記モード）
    with open(CSV_FILENAME, mode="a", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)

        sample_count = 0   # ★サンプルカウンタ

        try:
            while True:
                line = ser.readline().decode(errors="ignore").strip()
                if not line:
                    continue  # タイムアウト等で空行のとき

                parts = line.split(',')

                if len(parts) != NUM_FEATURES:
                    print(f"Skip line (len={len(parts)}): {line}")
                    continue

                try:
                    values = [int(p) for p in parts]
                except ValueError:
                    print(f"Invalid line: {line}")
                    continue

                writer.writerow(values)
                f.flush()

                # ★サンプル数カウント
                sample_count += 1

                # ★コンソールに表示
                print(f"[{sample_count}] Received:", values)

                # ★目標値に達したときの通知（数値は変数から生成）
                if sample_count == TARGET_SAMPLES:
                    print(f"=== 目標の {TARGET_SAMPLES} サンプルに到達しました！ ===")

        except KeyboardInterrupt:
            print("\n終了します。")

    ser.close()
    print("Serial closed.")

if __name__ == "__main__":
    main()
