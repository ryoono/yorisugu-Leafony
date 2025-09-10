import tkinter as tk

# ===== 設定 =====
CELL_SIZE   = 28         # 1マスのピクセル
DOT_ROWS    = 7          # 各小ブロックの縦ドット数（CGRAMは8行だが最下段は自動で0を追加）
DOT_COLS    = 5          # 各小ブロックの横ドット数
TILES_X     = 2          # 横方向の小ブロック数（上段 左/右）
TILES_Y     = 2          # 縦方向の小ブロック数（上段/下段）
GAP_PIX     = 8          # 小ブロック間の間隔（見やすさ用）
MSB_LEFT    = True       # True: 左端ドット=bit4（MSB） / False: 左端ドット=bit0（LSB）

BG_COLOR    = "#ffffff"
FG_COLOR    = "#000000"
GRID_COLOR  = "#999999"
TILE_BORDER = "#333333"


class DotMatrix2x2:
    def __init__(self, root):
        self.root = root
        self.root.title("2x2 Dot Matrix Editor (7x5 × 4 = 14x10)")

        # 全体キャンバスサイズ
        self.tile_w = DOT_COLS * CELL_SIZE
        self.tile_h = DOT_ROWS * CELL_SIZE
        total_w = TILES_X * self.tile_w + (TILES_X - 1) * GAP_PIX
        total_h = TILES_Y * self.tile_h + (TILES_Y - 1) * GAP_PIX

        self.canvas = tk.Canvas(root, width=total_w, height=total_h, bg=BG_COLOR, highlightthickness=0)
        self.canvas.grid(row=0, column=0, columnspan=3, padx=10, pady=10)

        # 4タイル分の状態 [tile_index][row][col]  (tile_index: 0=UL,1=UR,2=LL,3=LR)
        self.tiles = [
            [[0 for _ in range(DOT_COLS)] for _ in range(DOT_ROWS)]
            for _ in range(TILES_X * TILES_Y)
        ]
        self.rects = [  # 同じ構造で矩形ID保持
            [[None for _ in range(DOT_COLS)] for _ in range(DOT_ROWS)]
            for _ in range(TILES_X * TILES_Y)
        ]

        # 描画
        self._draw_tiles()

        # クリック/ドラッグ
        self.canvas.bind("<Button-1>", self.on_click)
        self.canvas.bind("<B1-Motion>", self.on_drag)

        # ボタン群
        tk.Button(root, text="クリア", command=self.clear).grid(row=1, column=0, pady=(0, 10))
        tk.Button(root, text="CGRAMを出力", command=self.export_cgram).grid(row=1, column=1, pady=(0, 10))
        tk.Button(root, text="現在の図形をコピー(テキスト)", command=self.copy_ascii).grid(row=1, column=2, pady=(0, 10))

        # 説明ラベル
        info = ("タイル割当:\n"
                "  [0] 上左  → CGRAM 0\n"
                "  [1] 上右  → CGRAM 1\n"
                "  [2] 下左  → CGRAM 2\n"
                "  [3] 下右  → CGRAM 3\n"
                "液晶上の並べ方（2行×2列）:\n"
                "  1行目: 0, 1\n"
                "  2行目: 2, 3")
        tk.Label(root, text=info, justify="left").grid(row=2, column=0, columnspan=3, sticky="w", padx=10, pady=(0,10))

        # ドラッグ塗り用のフラグ
        self._last_painted = set()

    def _tile_origin(self, tx, ty):
        """タイル(tx,ty)の左上座標（canvas上）"""
        x = tx * self.tile_w + tx * GAP_PIX
        y = ty * self.tile_h + ty * GAP_PIX
        return x, y

    def _draw_tiles(self):
        """タイル枠とグリッド矩形を描画"""
        for ty in range(TILES_Y):
            for tx in range(TILES_X):
                tile_idx = ty * TILES_X + tx
                ox, oy = self._tile_origin(tx, ty)

                # 外枠
                self.canvas.create_rectangle(
                    ox, oy, ox + self.tile_w, oy + self.tile_h,
                    outline=TILE_BORDER, width=2
                )

                # セル描画
                for r in range(DOT_ROWS):
                    for c in range(DOT_COLS):
                        x1 = ox + c * CELL_SIZE
                        y1 = oy + r * CELL_SIZE
                        x2 = x1 + CELL_SIZE
                        y2 = y1 + CELL_SIZE
                        rect = self.canvas.create_rectangle(
                            x1, y1, x2, y2,
                            fill=BG_COLOR, outline=GRID_COLOR
                        )
                        self.rects[tile_idx][r][c] = rect

    def _hit_test(self, x, y):
        """
        キャンバス座標(x,y) → (tile_idx, r, c) を返す
        範囲外なら None
        """
        # どのタイルか
        for ty in range(TILES_Y):
            for tx in range(TILES_X):
                tile_idx = ty * TILES_X + tx
                ox, oy = self._tile_origin(tx, ty)
                if ox <= x < ox + self.tile_w and oy <= y < oy + self.tile_h:
                    # セル位置
                    cx = int((x - ox) // CELL_SIZE)
                    cy = int((y - oy) // CELL_SIZE)
                    if 0 <= cx < DOT_COLS and 0 <= cy < DOT_ROWS:
                        return tile_idx, cy, cx
        return None

    def _paint_cell(self, tile_idx, r, c, force=None):
        """
        セルを反転/塗りつぶし
        force: None→トグル, 0/1→その値に設定
        """
        key = (tile_idx, r, c)
        if force is None:
            self.tiles[tile_idx][r][c] ^= 1
        else:
            # ドラッグ時の無駄な再描画防止
            if key in self._last_painted and self.tiles[tile_idx][r][c] == force:
                return
            self.tiles[tile_idx][r][c] = force

        fill = FG_COLOR if self.tiles[tile_idx][r][c] else BG_COLOR
        self.canvas.itemconfig(self.rects[tile_idx][r][c], fill=fill)
        self._last_painted.add(key)

    def on_click(self, ev):
        self._last_painted.clear()
        hit = self._hit_test(ev.x, ev.y)
        if hit is None:
            return
        tile_idx, r, c = hit
        self._paint_cell(tile_idx, r, c, force=None)

    def on_drag(self, ev):
        hit = self._hit_test(ev.x, ev.y)
        if hit is None:
            return
        tile_idx, r, c = hit
        # ドラッグは「黒で塗る」挙動（Shift押しなら白で消す）
        force_val = 0 if (ev.state & 0x0001) else 1  # Shiftで白
        self._paint_cell(tile_idx, r, c, force=force_val)

    def clear(self):
        for t in range(TILES_X * TILES_Y):
            for r in range(DOT_ROWS):
                for c in range(DOT_COLS):
                    self.tiles[t][r][c] = 0
                    self.canvas.itemconfig(self.rects[t][r][c], fill=BG_COLOR)

    # ---- 変換・出力系 ----
    def _row_to_byte(self, row_bits):
        """
        row_bits: [0/1]*5 （左→右）
        戻り値: 0..31 の整数（HD44780 1行分）
        MSB_LEFT=True の場合: 左端=bit4（MSB）
        """
        v = 0
        for i, bit in enumerate(row_bits):
            if MSB_LEFT:
                # 左端→bit4, 右端→bit0
                pos = 4 - i
            else:
                # 左端→bit0, 右端→bit4
                pos = i
            if bit:
                v |= (1 << pos)
        return v

    def _tile_to_cgram_bytes(self, tile_idx):
        """
        指定タイルの 7行×5列 → 8バイト（最下段0でパディング）
        """
        bytes8 = []
        for r in range(DOT_ROWS):
            row_bits = self.tiles[tile_idx][r]
            bytes8.append(self._row_to_byte(row_bits))
        bytes8.append(0x00)  # 8行目は0（カーソル行/余白）
        return bytes8

    def _big_ascii_art(self):
        """
        2x2 タイルを結合した 14×10 の ASCII プレビューを返す（■/□）
        """
        H = DOT_ROWS * TILES_Y
        W = DOT_COLS * TILES_X
        lines = []
        for ry in range(H):
            # どのタイルの行？
            ty = ry // DOT_ROWS
            in_r = ry % DOT_ROWS
            row_cells = []
            for rx in range(W):
                tx = rx // DOT_COLS
                in_c = rx % DOT_COLS
                tile_idx = ty * TILES_X + tx
                val = self.tiles[tile_idx][in_r][in_c]
                row_cells.append("■" if val else "□")
            lines.append("".join(row_cells))
        return "\n".join(lines)

    def export_cgram(self):
        """
        4文字分のCGRAM配列（8バイト×4）を16進で出力
        0:上左, 1:上右, 2:下左, 3:下右
        """
        print("="*50)
        print("CGRAM bytes (HD44780 5x8, MSB_LEFT={}):".format(MSB_LEFT))
        for idx, label in enumerate(["UL→CGRAM0", "UR→CGRAM1", "LL→CGRAM2", "LR→CGRAM3"]):
            arr = self._tile_to_cgram_bytes(idx)
            hexs = ", ".join(f"0x{b:02X}" for b in arr)
            print(f"[{label}]  {{ {hexs} }}")
        print("-"*50)
        print("Big glyph preview (14x10):")
        print(self._big_ascii_art())
        print("-"*50)
        print("配置例（16x2表示）:")
        print("  1行目: CGRAM0, CGRAM1")
        print("  2行目: CGRAM2, CGRAM3")
        print("="*50)

    def copy_ascii(self):
        """合成プレビューをクリップボードへ"""
        text = self._big_ascii_art()
        self.root.clipboard_clear()
        self.root.clipboard_append(text)
        self.root.update()
        # 軽いフィードバック（タイトル一時変更）
        old = self.root.title()
        self.root.title("コピーしました！")
        self.root.after(800, lambda: self.root.title(old))


if __name__ == "__main__":
    root = tk.Tk()
    app = DotMatrix2x2(root)
    root.mainloop()
