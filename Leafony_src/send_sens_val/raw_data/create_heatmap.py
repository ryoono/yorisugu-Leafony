import numpy as np
import matplotlib.pyplot as plt

# 6x3 heatmap data
datasets = [
    [233, 97, 203, 233, 26, 191, 137, 42, 22, 20, 23, 26, 209, 184, 166, 27, 16, 18],
    [227, 30, 201, 190, 28, 45, 42, 21, 26, 34, 54, 72, 28, 16, 22, 223, 178, 147],
    [262, 206, 24, 83, 238, 264, 270, 179, 22, 23, 25, 205, 209, 54, 17, 21, 15, 24],
    [262, 210, 237, 137, 29, 221, 269, 218, 32, 20, 21, 55, 207, 25, 19, 16, 18, 30]
]

titles = [
    "Kinoko Right",
    "Kinoko Left",
    "Takenoko Right",
    "Takenoko Left"
]

# Convert each dataset into 3 rows x 6 columns
heatmaps = [np.array(d).reshape(3, 6) for d in datasets]

# Unified color scale
vmin = min(min(d) for d in datasets)
vmax = max(max(d) for d in datasets)
threshold = (vmin + vmax) / 2  # threshold for text color switching

# Create 2x2 layout
fig, axes = plt.subplots(2, 2, figsize=(12, 7))
axes = axes.flatten()

for ax, hm, title in zip(axes, heatmaps, titles):
    im = ax.imshow(hm, vmin=vmin, vmax=vmax)
    ax.set_title(title)
    ax.set_xticks(range(hm.shape[1]))
    ax.set_yticks(range(hm.shape[0]))

    # Draw numbers at the center of each cell
    for i in range(hm.shape[0]):
        for j in range(hm.shape[1]):
            value = hm[i, j]
            text_color = "white" if value < threshold else "black"
            ax.text(
                j, i, str(value),
                ha="center", va="center",
                fontsize=10, color=text_color
            )

# Keep the layout fixed and place the colorbar at the far right
fig.subplots_adjust(right=0.85)
cbar_ax = fig.add_axes([0.88, 0.15, 0.02, 0.7])
cbar = fig.colorbar(im, cax=cbar_ax)
cbar.set_label("Value")

plt.show()