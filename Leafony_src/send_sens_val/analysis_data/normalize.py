# make_x_std.py

import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler

# ==============================
# 設定
# ==============================
RAW_CSV_PATH   = "../raw_data/kinoko_takenoko_merged.csv"   # 生データ (label, adc0..adc17)
Y_NPY_PATH     = "y.npy"                      # ラベル保存先
XSTD_NPY_PATH  = "X_std.npy"                  # 標準化済み特徴量保存先
SCALER_NPZ_PATH = "scaler_params.npz"         # mean / scale 保存先
NORM_CSV_PATH  = "normalized_from_raw.csv"    # 確認用(ラベル + 標準化特徴量)

# ==============================
# データ読み込み
# ==============================
df = pd.read_csv(RAW_CSV_PATH, header=None)

# 0列目がラベル, 1〜18列目が特徴量（生の analogRead 値）
y = df.iloc[:, 0].astype(int).values          # shape: (N,)
X_raw = df.iloc[:, 1:].values                 # shape: (N, 18)

print(f"Loaded raw data: X_raw={X_raw.shape}, y={y.shape}")

# ==============================
# 標準化 (StandardScaler)
# ==============================
scaler = StandardScaler()
X_std = scaler.fit_transform(X_raw)           # shape: (N, 18)

print("Standardization done.")
print(f"mean:  {scaler.mean_}")
print(f"scale: {scaler.scale_}")

# ==============================
# 保存
# ==============================

# ラベルと標準化済み特徴量を .npy で保存
np.save(Y_NPY_PATH, y)
np.save(XSTD_NPY_PATH, X_std)

# scaler の mean / scale も保存（後で w* / b* 計算に使う）
np.savez(SCALER_NPZ_PATH, mean=scaler.mean_, scale=scaler.scale_)

# 確認用に CSV も書き出し（ラベル + 標準化特徴量）
norm_data = np.column_stack([y, X_std])       # shape: (N, 1+18)
norm_df = pd.DataFrame(norm_data)
norm_df.to_csv(NORM_CSV_PATH, header=False, index=False)

print(f"Saved y -> {Y_NPY_PATH}")
print(f"Saved X_std -> {XSTD_NPY_PATH}")
print(f"Saved scaler params -> {SCALER_NPZ_PATH}")
print(f"Saved normalized CSV -> {NORM_CSV_PATH}")
