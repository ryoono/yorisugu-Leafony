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

# Convert each dataset into 3 rows x 6 columns
heatmaps = [np.array(d).reshape(3, 6) for d in datasets]

# Unified color scale
vmin = min(min(d) for d in datasets)
vmax = max(max(d) for d in datasets)
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
                j, i, str(value),
                ha="center", va="center",
                fontsize=10, color=text_color
            )

# カラーバー
fig.subplots_adjust(right=0.85)
cbar_ax = fig.add_axes([0.88, 0.15, 0.02, 0.7])
cbar = fig.colorbar(im, cax=cbar_ax)
cbar.set_label("Value")

plt.show()