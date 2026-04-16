import numpy as np
import matplotlib.pyplot as plt

# 6x3 heatmap data
datasets = [
    [287,224,267,34,54,156,290,211,224,17,23,82,129,36,44,0,34,82],
    [266,56,153,207,10,205,99,5,5,4,3,5,186,191,171,12,3,5]
]

titles = [
    "ambient light",
    "No ambient light"
]

# ==============================
# ★ scaler読み込み（学習と同じ）
# ==============================
scaler = np.load("scaler_params.npz")
mean = scaler["mean"]    # shape (18,)
scale = scaler["scale"]  # shape (18,)

# ==============================
# ★ 標準化関数（StandardScalerと同じ）
# ==============================
def standardize(data):
    data = np.array(data)
    return (data - mean) / scale

# 標準化してreshape
heatmaps = [standardize(d).reshape(3, 6) for d in datasets]

# カラースケール統一
vmin = min(hm.min() for hm in heatmaps)
vmax = max(hm.max() for hm in heatmaps)
threshold = (vmin + vmax) / 2

fig, axes = plt.subplots(1, 2, figsize=(10, 4))

for ax, hm, title in zip(axes, heatmaps, titles):
    im = ax.imshow(hm, vmin=vmin, vmax=vmax)
    ax.set_title(title)
    ax.set_xticks(range(hm.shape[1]))
    ax.set_yticks(range(hm.shape[0]))

    for i in range(hm.shape[0]):
        for j in range(hm.shape[1]):
            value = hm[i, j]
            text_color = "white" if value < threshold else "black"
            ax.text(
                j, i, f"{value:.2f}",
                ha="center", va="center",
                fontsize=10, color=text_color
            )

# カラーバー
fig.subplots_adjust(right=0.85)
cbar_ax = fig.add_axes([0.88, 0.15, 0.02, 0.7])
cbar = fig.colorbar(im, cax=cbar_ax)
cbar.set_label("Standardized Value")

plt.show()