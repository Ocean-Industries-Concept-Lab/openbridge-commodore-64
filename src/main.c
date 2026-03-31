#define SCREEN_RAM   ((unsigned char*)0x4000)
#define BITMAP_RAM   ((unsigned char*)0x6000)

#define VIC_BANK_SEL ((unsigned char*)0xDD00)
#define RASTER       ((unsigned char*)0xD012)
#define CONTROL1     ((unsigned char*)0xD011)
#define CONTROL2     ((unsigned char*)0xD016)
#define MEMORY_SETUP ((unsigned char*)0xD018)

#define BORDER_COLOR ((unsigned char*)0xD020)
#define BG_COLOR0    ((unsigned char*)0xD021)

#define GETIN        ((unsigned char*)0xFFE4)

#define COLOR_WHITE  0x01
#define COLOR_BLACK  0x00
#define COLOR_DGREY  0x0B
#define COLOR_DBLUE  0x06
#define COLOR_LGREY  0x0F
#define COLOR_LBLUE  0x0E

#define SCREEN_W     320
#define SCREEN_H     200
#define COMPASS_CY   (SCREEN_H / 2)
#define COMPASS_R    ((SCREEN_H / 2) - 8)
#define COMPASS_CX   (SCREEN_W - COMPASS_R - 6)

static const signed char unit_x[36] = {
      0,  11,  22,  32,  41,  49,  55,  60,  63,
     64,  63,  60,  55,  49,  41,  32,  22,  11,
      0, -11, -22, -32, -41, -49, -55, -60, -63,
    -64, -63, -60, -55, -49, -41, -32, -22, -11
};

static const signed char unit_y[36] = {
    -64, -63, -60, -55, -49, -41, -32, -22, -11,
      0,  11,  22,  32,  41,  49,  55,  60,  63,
     64,  63,  60,  55,  49,  41,  32,  22,  11,
      0, -11, -22, -32, -41, -49, -55, -60, -63
};

static const unsigned char seg_map[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

static const unsigned char glyph_c[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
static const unsigned char glyph_o[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
static const unsigned char glyph_m[7] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
static const unsigned char glyph_p[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
static const unsigned char glyph_a[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
static const unsigned char glyph_s[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
static const unsigned char glyph_h[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
static const unsigned char glyph_d[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
static const unsigned char glyph_e[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
static const unsigned char glyph_g[7] = {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E};
static const unsigned char glyph_r[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
static const unsigned char glyph_t[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};

static void draw_big_digit(unsigned char x, unsigned char y, unsigned char digit);

static void wait_frame(void)
{
    while (*RASTER != 0xFFu) {
    }
    while (*RASTER == 0xFFu) {
    }
}

static void wait_keypress(void)
{
    while (((unsigned char (*)(void))GETIN)() == 0u) {
        wait_frame();
    }
}

static unsigned int bitmap_index(unsigned int x, unsigned char y)
{
    return (unsigned int)(y >> 3) * 320u + (x >> 3) * 8u + (unsigned int)(y & 7u);
}

static void set_cell_colors(unsigned char cell_x, unsigned char cell_y, unsigned char color0, unsigned char color1)
{
    SCREEN_RAM[(unsigned int)cell_y * 40u + cell_x] = (unsigned char)((color0 << 4) | color1);
}

static void set_region_colors(unsigned char x, unsigned char y, unsigned char w, unsigned char h, unsigned char color0, unsigned char color1)
{
    unsigned char cell_x0 = (unsigned char)(x >> 3);
    unsigned char cell_y0 = (unsigned char)(y >> 3);
    unsigned char cell_x1 = (unsigned char)((x + w - 1u) >> 3);
    unsigned char cell_y1 = (unsigned char)((y + h - 1u) >> 3);
    unsigned char cx;
    unsigned char cy;

    for (cy = cell_y0; cy <= cell_y1; ++cy) {
        for (cx = cell_x0; cx <= cell_x1; ++cx) {
            set_cell_colors(cx, cy, color0, color1);
        }
    }
}

static void put_pixel(signed int x, signed int y, unsigned char set)
{
    unsigned int idx;
    unsigned char mask;

    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) {
        return;
    }

    idx = bitmap_index((unsigned int)x, (unsigned char)y);
    mask = (unsigned char)(0x80u >> ((unsigned int)x & 7u));

    if (set) {
        BITMAP_RAM[idx] |= mask;
    } else {
        BITMAP_RAM[idx] &= (unsigned char)~mask;
    }
}

static void draw_line(signed int x0, signed int y0, signed int x1, signed int y1, unsigned char set)
{
    signed int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    signed int sx = (x0 < x1) ? 1 : -1;
    signed int dy = (y1 > y0) ? -(y1 - y0) : -(y0 - y1);
    signed int sy = (y0 < y1) ? 1 : -1;
    signed int err = dx + dy;

    for (;;) {
        put_pixel(x0, y0, set);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        {
            signed int e2 = err << 1;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
}

static void fill_rect(signed int x, signed int y, unsigned char w, unsigned char h, unsigned char set)
{
    unsigned char yy;
    unsigned char xx;

    for (yy = 0; yy < h; ++yy) {
        for (xx = 0; xx < w; ++xx) {
            put_pixel(x + (signed int)xx, y + (signed int)yy, set);
        }
    }
}

static void draw_glyph5x7(signed int x, signed int y, const unsigned char* glyph)
{
    unsigned char row;
    for (row = 0; row < 7u; ++row) {
        unsigned char bits = glyph[row];
        unsigned char col;
        for (col = 0; col < 5u; ++col) {
            if (bits & (unsigned char)(0x10u >> col)) {
                put_pixel(x + (signed int)col, y + (signed int)row, 1);
            }
        }
    }
}

static void draw_small_text(signed int x, signed int y, const char* text)
{
    signed int cursor = x;
    while (*text != '\0') {
        const unsigned char* glyph = 0;
        switch (*text) {
            case 'C': glyph = glyph_c; break;
            case 'O': glyph = glyph_o; break;
            case 'M': glyph = glyph_m; break;
            case 'P': glyph = glyph_p; break;
            case 'A': glyph = glyph_a; break;
            case 'S': glyph = glyph_s; break;
            case 'H': glyph = glyph_h; break;
            case 'D': glyph = glyph_d; break;
            case 'E': glyph = glyph_e; break;
            case 'G': glyph = glyph_g; break;
            case 'R': glyph = glyph_r; break;
            case 'T': glyph = glyph_t; break;
            default: break;
        }

        if (glyph != 0) {
            draw_glyph5x7(cursor, y, glyph);
        }
        cursor += 6;
        ++text;
    }
}

static void draw_circle(signed int cx, signed int cy, unsigned char r)
{
    signed int x = r;
    signed int y = 0;
    signed int err = 0;

    while (x >= y) {
        put_pixel(cx + x, cy + y, 1);
        put_pixel(cx + y, cy + x, 1);
        put_pixel(cx - y, cy + x, 1);
        put_pixel(cx - x, cy + y, 1);
        put_pixel(cx - x, cy - y, 1);
        put_pixel(cx - y, cy - x, 1);
        put_pixel(cx + y, cy - x, 1);
        put_pixel(cx + x, cy - y, 1);

        ++y;
        err += (y << 1) + 1;
        if ((err << 1) + 1 > (x << 1)) {
            --x;
            err -= (x << 1) + 1;
        }
    }
}

static signed int scale_component(signed int unit, unsigned char radius)
{
    signed int value = unit * (signed int)radius;
    if (value >= 0) {
        return (value + 32) >> 6;
    }
    return -(((-value) + 32) >> 6);
}

static signed int interp_unit(const signed char* table, unsigned int angle_deg)
{
    unsigned char base = (unsigned char)(angle_deg / 10u);
    unsigned char frac = (unsigned char)(angle_deg % 10u);
    unsigned char next = (unsigned char)((base + 1u) % 36u);
    signed int a = (signed int)table[base];
    signed int b = (signed int)table[next];

    return (a * (signed int)(10u - frac) + b * (signed int)frac) / 10;
}

static signed int rotate_x(signed int x, signed int y, unsigned int angle_deg)
{
    signed int ux = interp_unit(unit_x, angle_deg % 360u);
    signed int uy = interp_unit(unit_y, angle_deg % 360u);
    return (x * ux - y * uy) / 64;
}

static signed int rotate_y(signed int x, signed int y, unsigned int angle_deg)
{
    signed int ux = interp_unit(unit_x, angle_deg % 360u);
    signed int uy = interp_unit(unit_y, angle_deg % 360u);
    return (x * uy + y * ux) / 64;
}

static void draw_filled_triangle(signed int x1, signed int y1, signed int x2, signed int y2, signed int x3, signed int y3, unsigned char set)
{
    unsigned char i;

    for (i = 0; i <= 16u; ++i) {
        signed int xa = x2 + ((x1 - x2) * (signed int)i) / 16;
        signed int ya = y2 + ((y1 - y2) * (signed int)i) / 16;
        signed int xb = x3 + ((x1 - x3) * (signed int)i) / 16;
        signed int yb = y3 + ((y1 - y3) * (signed int)i) / 16;
        draw_line(xa, ya, xb, yb, set);
    }
}

static void draw_outward_pointer(unsigned int angle_deg, unsigned char base_r, unsigned char tip_r, unsigned char half_w, unsigned char filled)
{
    signed int ux = interp_unit(unit_x, angle_deg % 360u);
    signed int uy = interp_unit(unit_y, angle_deg % 360u);
    signed int px = interp_unit(unit_x, (angle_deg + 90u) % 360u);
    signed int py = interp_unit(unit_y, (angle_deg + 90u) % 360u);

    signed int tip_x = COMPASS_CX + scale_component(ux, tip_r);
    signed int tip_y = COMPASS_CY + scale_component(uy, tip_r);
    signed int base_x = COMPASS_CX + scale_component(ux, base_r);
    signed int base_y = COMPASS_CY + scale_component(uy, base_r);

    signed int left_x = base_x + scale_component(px, half_w);
    signed int left_y = base_y + scale_component(py, half_w);
    signed int right_x = base_x - scale_component(px, half_w);
    signed int right_y = base_y - scale_component(py, half_w);

    if (filled) {
        draw_filled_triangle(tip_x, tip_y, left_x, left_y, right_x, right_y, 1);
    } else {
        draw_line(tip_x, tip_y, left_x, left_y, 1);
        draw_line(tip_x, tip_y, right_x, right_y, 1);
        draw_line(left_x, left_y, right_x, right_y, 1);
    }
}

static void draw_filled_circle(signed int cx, signed int cy, unsigned char r, unsigned char set)
{
    signed int y;
    signed int rr = (signed int)r * (signed int)r;

    for (y = -(signed int)r; y <= (signed int)r; ++y) {
        signed int x = 0;
        while ((x * x + y * y) <= rr) {
            ++x;
        }
        --x;
        if (x >= 0) {
            draw_line(cx - x, cy + y, cx + x, cy + y, set);
        }
    }
}

static void draw_rotating_arrow(unsigned int angle_deg)
{
    signed int cx = COMPASS_CX;
    signed int cy = COMPASS_CY;
    signed int ux = interp_unit(unit_x, angle_deg % 360u);
    signed int uy = interp_unit(unit_y, angle_deg % 360u);
    signed int px = interp_unit(unit_x, (angle_deg + 90u) % 360u);
    signed int py = interp_unit(unit_y, (angle_deg + 90u) % 360u);

    signed int tip_x = cx + scale_component(ux, (unsigned char)(COMPASS_R - 2));
    signed int tip_y = cy + scale_component(uy, (unsigned char)(COMPASS_R - 2));
    signed int base_x = cx + scale_component(ux, (unsigned char)(COMPASS_R - 22));
    signed int base_y = cy + scale_component(uy, (unsigned char)(COMPASS_R - 22));

    signed int left_x = base_x + scale_component(px, 5);
    signed int left_y = base_y + scale_component(py, 5);
    signed int right_x = base_x - scale_component(px, 5);
    signed int right_y = base_y - scale_component(py, 5);

    draw_filled_triangle(tip_x, tip_y, left_x, left_y, right_x, right_y, 1);
}

static void draw_boat_outline(unsigned int angle_deg)
{
    signed int cx = COMPASS_CX;
    signed int cy = COMPASS_CY;

    signed int p0x = rotate_x(0, -28, angle_deg);
    signed int p0y = rotate_y(0, -28, angle_deg);
    signed int p1x = rotate_x(-9, -12, angle_deg);
    signed int p1y = rotate_y(-9, -12, angle_deg);
    signed int p2x = rotate_x(-6, 22, angle_deg);
    signed int p2y = rotate_y(-6, 22, angle_deg);
    signed int p3x = rotate_x(6, 22, angle_deg);
    signed int p3y = rotate_y(6, 22, angle_deg);
    signed int p4x = rotate_x(9, -12, angle_deg);
    signed int p4y = rotate_y(9, -12, angle_deg);

    draw_line(cx + p0x, cy + p0y, cx + p1x, cy + p1y, 1);
    draw_line(cx + p1x, cy + p1y, cx + p2x, cy + p2y, 1);
    draw_line(cx + p2x, cy + p2y, cx + p3x, cy + p3y, 1);
    draw_line(cx + p3x, cy + p3y, cx + p4x, cy + p4y, 1);
    draw_line(cx + p4x, cy + p4y, cx + p0x, cy + p0y, 1);

    draw_line(cx + rotate_x(-4, -18, angle_deg), cy + rotate_y(-4, -18, angle_deg),
              cx + rotate_x(4, -18, angle_deg), cy + rotate_y(4, -18, angle_deg), 1);
    draw_line(cx + rotate_x(-3, -2, angle_deg), cy + rotate_y(-3, -2, angle_deg),
              cx + rotate_x(3, -2, angle_deg), cy + rotate_y(3, -2, angle_deg), 1);
}

static void draw_static_scale(void)
{
    signed int cx = COMPASS_CX;
    signed int cy = COMPASS_CY;
    unsigned int angle = 0;
    unsigned char tick_inner = (unsigned char)(COMPASS_R - 29);

    for (angle = 0; angle < 360u; angle += 90u) {
        signed int ux = interp_unit(unit_x, angle);
        signed int uy = interp_unit(unit_y, angle);
        signed int x1 = cx + scale_component(ux, (unsigned char)(COMPASS_R - 3));
        signed int y1 = cy + scale_component(uy, (unsigned char)(COMPASS_R - 3));
        signed int x2 = cx + scale_component(ux, tick_inner);
        signed int y2 = cy + scale_component(uy, tick_inner);

        draw_line(x1, y1, x2, y2, 1);
    }
}

static void draw_compass_dynamic(unsigned int heading_deg)
{
    static const unsigned int dot_angles[5] = {306u, 18u, 90u, 162u, 234u};
    unsigned char tick_inner = (unsigned char)(COMPASS_R - 29);
    unsigned char ring3 = 42;
    unsigned char ring4 = tick_inner;
    unsigned char mid_outer = 42;
    unsigned char mid_inner = 30;
    unsigned char i;

    fill_rect(COMPASS_CX - COMPASS_R - 4, COMPASS_CY - COMPASS_R - 4, (unsigned char)(COMPASS_R * 2 + 9), (unsigned char)(COMPASS_R * 2 + 9), 0);
    draw_circle(COMPASS_CX, COMPASS_CY, (unsigned char)(COMPASS_R + 2));
    draw_circle(COMPASS_CX, COMPASS_CY, COMPASS_R);
    draw_circle(COMPASS_CX, COMPASS_CY, tick_inner);
    draw_circle(COMPASS_CX, COMPASS_CY, mid_outer);
    draw_circle(COMPASS_CX, COMPASS_CY, mid_inner);

    for (i = 0; i < 5u; ++i) {
        signed int ux = interp_unit(unit_x, dot_angles[i]);
        signed int uy = interp_unit(unit_y, dot_angles[i]);
        signed int dx = COMPASS_CX + scale_component(ux, 36);
        signed int dy = COMPASS_CY + scale_component(uy, 36);
        draw_filled_circle(dx, dy, 3, 1);
    }

    draw_circle(COMPASS_CX, COMPASS_CY, 3);
    draw_static_scale();
    draw_outward_pointer(316u, (unsigned char)(ring3 + 7), (unsigned char)(ring4 - 2), 5, 1);
    draw_outward_pointer(298u, (unsigned char)(ring3 + 8), (unsigned char)(ring4 - 1), 4, 0);
    draw_outward_pointer(0u, (unsigned char)(COMPASS_R - 8), (unsigned char)(COMPASS_R + 1), 4, 1);
    draw_outward_pointer(300u, (unsigned char)(COMPASS_R - 3), (unsigned char)(COMPASS_R - 14), 4, 1);
    draw_boat_outline((heading_deg + 20u) % 360u);
    draw_filled_circle(314, 8, 1, 1);
    draw_filled_circle(314, 16, 1, 1);
}

static void draw_value(unsigned char x, unsigned char y, unsigned int value_deg)
{
    unsigned char d2 = (unsigned char)(value_deg / 100u);
    unsigned char d1 = (unsigned char)((value_deg / 10u) % 10u);
    unsigned char d0 = (unsigned char)(value_deg % 10u);

    fill_rect(x - 4, y - 4, 56, 26, 0);

    draw_big_digit(x, y, d2);
    draw_big_digit((unsigned char)(x + 15), y, d1);
    draw_big_digit((unsigned char)(x + 30), y, d0);
}

static void draw_segment_h(unsigned char x, unsigned char y)
{
    fill_rect((unsigned char)(x + 3), y, 8, 2, 1);
}

static void draw_segment_v(unsigned char x, unsigned char y)
{
    fill_rect(x, (unsigned char)(y + 2), 2, 8, 1);
}

static void draw_big_digit(unsigned char x, unsigned char y, unsigned char digit)
{
    unsigned char mask = seg_map[digit];

    if (mask & 0x01) {
        draw_segment_h(x, y);
    }
    if (mask & 0x02) {
        draw_segment_v((unsigned char)(x + 11), y);
    }
    if (mask & 0x04) {
        draw_segment_v((unsigned char)(x + 11), (unsigned char)(y + 11));
    }
    if (mask & 0x08) {
        draw_segment_h(x, (unsigned char)(y + 22));
    }
    if (mask & 0x10) {
        draw_segment_v((unsigned char)(x + 1), (unsigned char)(y + 11));
    }
    if (mask & 0x20) {
        draw_segment_v((unsigned char)(x + 1), y);
    }
    if (mask & 0x40) {
        draw_segment_h(x, (unsigned char)(y + 11));
    }
}

static void draw_labels(void)
{
    fill_rect(8, 8, 88, 10, 0);
    fill_rect(8, 50, 72, 10, 0);
    fill_rect(8, 104, 72, 10, 0);
    fill_rect(8, 158, 72, 10, 0);
    draw_small_text(10, 10, "COMPASS");
    draw_small_text(10, 52, "HDG  DEG");
    draw_small_text(10, 106, "COG  DEG");
    draw_small_text(10, 160, "ROT  DEG");
}

static unsigned char map_color(unsigned char color, unsigned char inverted)
{
    if (!inverted) {
        return color;
    }

    if (color == COLOR_WHITE) {
        return COLOR_BLACK;
    }
    if (color == COLOR_BLACK) {
        return COLOR_WHITE;
    }
    if (color == COLOR_DBLUE) {
        return COLOR_LBLUE;
    }
    if (color == COLOR_DGREY) {
        return COLOR_LGREY;
    }

    return color;
}

static void apply_color_scheme(unsigned char inverted)
{
    unsigned int i;
    unsigned char bg = map_color(COLOR_WHITE, inverted);
    unsigned char base = map_color(COLOR_DBLUE, inverted);

    *BORDER_COLOR = bg;
    *BG_COLOR0 = bg;

    for (i = 0; i < 1000u; ++i) {
        SCREEN_RAM[i] = (unsigned char)((base << 4) | bg);
    }

    set_region_colors(8, 8, 88, 10, map_color(COLOR_BLACK, inverted), bg);
    set_region_colors(10, 22, 56, 26, map_color(COLOR_DBLUE, inverted), bg);
    set_region_colors(10, 76, 56, 26, map_color(COLOR_DGREY, inverted), bg);
    set_region_colors(10, 130, 56, 26, map_color(COLOR_DGREY, inverted), bg);
    set_region_colors(8, 50, 72, 10, map_color(COLOR_DGREY, inverted), bg);
    set_region_colors(8, 104, 72, 10, map_color(COLOR_DGREY, inverted), bg);
    set_region_colors(8, 158, 72, 10, map_color(COLOR_DGREY, inverted), bg);
    set_region_colors((unsigned char)(COMPASS_CX - 66), (unsigned char)(COMPASS_CY - 66), 132, 132, map_color(COLOR_DGREY, inverted), bg);
    set_region_colors((unsigned char)(COMPASS_CX - 62), (unsigned char)(COMPASS_CY - 68), 22, 24, map_color(COLOR_DBLUE, inverted), bg);
    set_region_colors(310, 6, 8, 14, map_color(COLOR_DGREY, inverted), bg);
}

static void setup_bitmap(void)
{
    unsigned int i;

    *VIC_BANK_SEL = (unsigned char)((*VIC_BANK_SEL & 0xFC) | 0x02);
    *MEMORY_SETUP = 0x08;
    *CONTROL1 = (unsigned char)((*CONTROL1 & 0x9F) | 0x20);
    *CONTROL2 = (unsigned char)(*CONTROL2 & 0xEF);

    for (i = 0; i < 8000u; ++i) {
        BITMAP_RAM[i] = 0x00;
    }

    apply_color_scheme(0);
}

int main(void)
{
    unsigned int heading_deg = 231;
    unsigned int cog_deg = (heading_deg + 14u) % 360u;
    unsigned int rot_deg = 3u;
    unsigned char inverted = 0;

    setup_bitmap();
    draw_labels();
    draw_value(14, 26, heading_deg);
    draw_value(14, 80, cog_deg);
    draw_value(14, 134, rot_deg);
    draw_compass_dynamic(heading_deg);

    for (;;) {
        wait_keypress();

        inverted ^= 1u;
        apply_color_scheme(inverted);

        heading_deg = (heading_deg + 1u) % 360u;
        cog_deg = (heading_deg + 14u) % 360u;

        draw_value(14, 26, heading_deg);
        draw_value(14, 80, cog_deg);
        draw_value(14, 134, rot_deg);
        draw_compass_dynamic(heading_deg);
    }
}
