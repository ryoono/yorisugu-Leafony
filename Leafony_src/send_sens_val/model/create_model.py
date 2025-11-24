# train_and_export_header_from_xstd.py

import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score, classification_report

# ==============================
# 設定
# ==============================
Y_NPY_PATH       = "../analysis_data/y.npy"
XSTD_NPY_PATH    = "../analysis_data/X_std.npy"
SCALER_NPZ_PATH  = "../analysis_data/scaler_params.npz"
HEADER_PATH      = "logistic_params_raw.h"    # 生ADC値に対応した係数を出力

TEST_SIZE        = 0.3
RANDOM_STATE     = 42
NUM_CLASSES      = 4
NUM_FEATURES     = 18

# ==============================
# データ読み込み
# ==============================
y = np.load(Y_NPY_PATH)           # shape: (N,)
X_std = np.load(XSTD_NPY_PATH)    # shape: (N, 18)

scaler_params = np.load(SCALER_NPZ_PATH)
mean = scaler_params["mean"]      # shape: (18,)
scale = scaler_params["scale"]    # shape: (18,)

print(f"Loaded: y={y.shape}, X_std={X_std.shape}")
print(f"mean:  {mean}")
print(f"scale: {scale}")

# ==============================
# train/test 分割（X_std + y）
# ==============================
X_train, X_test, y_train, y_test = train_test_split(
    X_std,
    y,
    test_size=TEST_SIZE,
    shuffle=True,
    stratify=y,
    random_state=RANDOM_STATE,
)

print(f"Train: X={X_train.shape}, Test: X={X_test.shape}")

# ==============================
# 多クラスロジスティック回帰の学習
# ==============================
clf = LogisticRegression(
    penalty="l2",
    solver="lbfgs",
    multi_class="multinomial",
    max_iter=2000,
)

clf.fit(X_train, y_train)

# ==============================
# 精度評価
# ==============================
y_pred = clf.predict(X_test)
acc = accuracy_score(y_test, y_pred)

print("====================================")
print("Multiclass Logistic Regression (trained on X_std)")
print(f"Accuracy: {acc:.4f}")
print("====================================")
print("Classification report:")
print(classification_report(y_test, y_pred, digits=4))

# ==============================
# 標準化を吸収した w_star, b_star を計算
# ==============================
# clf.coef_ : shape (num_classes, num_features)  → W
# clf.intercept_ : shape (num_classes,)          → b
W = clf.coef_       # shape: (4, 18)
b = clf.intercept_  # shape: (4,)

assert W.shape == (NUM_CLASSES, NUM_FEATURES), W.shape
assert b.shape == (NUM_CLASSES,), b.shape

# w*_k,i = W_k,i / scale_i
w_star = W / scale  # ブロードキャストで (4,18) / (18,) になる

# b*_k = b_k - Σ_i W_k,i * mean_i / scale_i
b_star = b - (W * (mean / scale)).sum(axis=1)

# 確認用：X_raw からの予測と X_std モデルが一致するかチェックしたい場合は
# 別途 X_raw を読んでテストしてもよい

# ==============================
# Cヘッダファイルに書き出し
#   LOGI_W = w_star
#   LOGI_B = b_star
# ==============================
def format_c_array_2d(name: str, arr: np.ndarray) -> str:
    lines = []
    lines.append(f"// shape: {arr.shape[0]} x {arr.shape[1]}")
    lines.append(f"const float {name}[{arr.shape[0]}][{arr.shape[1]}] = {{")
    for r in range(arr.shape[0]):
        row_vals = ", ".join(f"{v:.6f}f" for v in arr[r])
        comma = "," if r < arr.shape[0] - 1 else ""
        lines.append(f"    {{{row_vals}}}{comma}")
    lines.append("};")
    return "\n".join(lines)

def format_c_array_1d(name: str, arr: np.ndarray) -> str:
    vals = ", ".join(f"{v:.6f}f" for v in arr)
    return f"// length: {arr.shape[0]}\nconst float {name}[{arr.shape[0]}] = {{ {vals} }};"

header_lines = []

header_lines.append("// Auto-generated logistic regression parameters (with standardization folded in)")
header_lines.append("// Classes:")
header_lines.append("//   0 = Kinoko (Right)")
header_lines.append("//   1 = Kinoko (Left)")
header_lines.append("//   2 = Takenoko (Right)")
header_lines.append("//   3 = Takenoko (Left)")
header_lines.append("")
header_lines.append(f"// Accuracy on test set (trained on X_std): {acc:.6f}")
header_lines.append("// NOTE:")
header_lines.append("//   These weights expect 18-dimensional INPUT FEATURES")
header_lines.append("//   that are RAW analogRead() values (0–1023),")
header_lines.append("//   because the effect of StandardScaler has been folded into LOGI_W / LOGI_B.")
header_lines.append("")
header_lines.append("#pragma once")
header_lines.append("")
header_lines.append(f"#define LOGI_NUM_CLASSES  {NUM_CLASSES}")
header_lines.append(f"#define LOGI_NUM_FEATURES {NUM_FEATURES}")
header_lines.append("")
header_lines.append(format_c_array_2d("LOGI_W", w_star))
header_lines.append("")
header_lines.append(format_c_array_1d("LOGI_B", b_star))
header_lines.append("")

with open(HEADER_PATH, "w", encoding="utf-8") as f:
    f.write("\n".join(header_lines))

print(f"\nHeader file written to: {HEADER_PATH}")
