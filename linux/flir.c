
// https://www-old.mpi-halle.mpg.de/mpi/publi/pdf/540_02.pdf

#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include <SDL.h>

#include "hidapi.h"
#include "golomb.h"

struct img {
    int w, h;
    double *pix;
};


struct ctx {
    SDL_Window *win;
    int win_w, win_h;
    double t;
    int n_cycle;
    int flir_w, flir_h;
    hid_device *handle;
    bool running;
    int n;
    const char *script;
    struct img *img_raw;
    struct img *img_re;
    struct img *img_im;
    struct img *img_abs;
    struct img *img_pha;
};


struct palette {
    uint32_t *rgba;
    size_t count;
};

static uint32_t colors_heatmap[] = { 0xff00000a, 0xff000014, 0xff00001e, 0xff000025, 0xff00002a, 0xff00002e, 0xff000032, 0xff000036, 0xff00003a, 0xff00003e, 0xff000042, 0xff000046, 0xff00004a, 0xff00004f, 0xff000052, 0xff010055, 0xff010057, 0xff020059, 0xff02005c, 0xff03005e, 0xff040061, 0xff040063, 0xff050065, 0xff060067, 0xff070069, 0xff08006b, 0xff09006e, 0xff0a0070, 0xff0b0073, 0xff0c0074, 0xff0d0075, 0xff0d0076, 0xff0e0077, 0xff100078, 0xff120079, 0xff13007b, 0xff15007c, 0xff17007d, 0xff19007e, 0xff1b0080, 0xff1c0081, 0xff1e0083, 0xff200084, 0xff220085, 0xff240086, 0xff260087, 0xff280089, 0xff2a0089, 0xff2c008a, 0xff2e008b, 0xff30008c, 0xff32008d, 0xff34008e, 0xff36008e, 0xff38008f, 0xff390090, 0xff3b0091, 0xff3c0092, 0xff3e0093, 0xff3f0093, 0xff410094, 0xff420095, 0xff440095, 0xff450096, 0xff470096, 0xff490096, 0xff4a0096, 0xff4c0097, 0xff4e0097, 0xff4f0097, 0xff510097, 0xff520098, 0xff540098, 0xff560098, 0xff580099, 0xff5a0099, 0xff5c0099, 0xff5d009a, 0xff5f009a, 0xff61009b, 0xff63009b, 0xff64009b, 0xff66009b, 0xff68009b, 0xff6a009b, 0xff6c009c, 0xff6d009c, 0xff6f009c, 0xff70009c, 0xff71009d, 0xff73009d, 0xff75009d, 0xff77009d, 0xff78009d, 0xff7a009d, 0xff7c009d, 0xff7e009d, 0xff7f009d, 0xff81009d, 0xff83009d, 0xff84009d, 0xff86009d, 0xff87009d, 0xff89009d, 0xff8a009d, 0xff8b009d, 0xff8d009d, 0xff8f009c, 0xff91009c, 0xff93009c, 0xff95009c, 0xff96009b, 0xff98009b, 0xff99009b, 0xff9b009b, 0xff9c009b, 0xff9d009b, 0xff9f009b, 0xffa0009b, 0xffa2009b, 0xffa3009b, 0xffa4009b, 0xffa6009a, 0xffa7009a, 0xffa8009a, 0xffa90099, 0xffaa0099, 0xffab0099, 0xffad0099, 0xffae0198, 0xffaf0198, 0xffb00198, 0xffb00198, 0xffb10197, 0xffb20197, 0xffb30196, 0xffb40296, 0xffb50295, 0xffb60295, 0xffb70395, 0xffb80395, 0xffb90495, 0xffba0495, 0xffba0494, 0xffbb0593, 0xffbc0593, 0xffbd0593, 0xffbe0692, 0xffbf0692, 0xffbf0692, 0xffc00791, 0xffc00791, 0xffc10890, 0xffc10990, 0xffc20a8f, 0xffc30a8e, 0xffc30b8e, 0xffc40c8d, 0xffc50c8c, 0xffc60d8b, 0xffc60e8a, 0xffc70f89, 0xffc81088, 0xffc91187, 0xffca1286, 0xffca1385, 0xffcb1385, 0xffcb1484, 0xffcc1582, 0xffcd1681, 0xffce1780, 0xffce187e, 0xffcf187c, 0xffcf197b, 0xffd01a79, 0xffd11b78, 0xffd11c76, 0xffd21c75, 0xffd21d74, 0xffd31e72, 0xffd32071, 0xffd4216f, 0xffd4226e, 0xffd5236b, 0xffd52469, 0xffd62567, 0xffd72665, 0xffd82764, 0xffd82862, 0xffd92a60, 0xffda2b5e, 0xffda2c5c, 0xffdb2e5a, 0xffdb2f57, 0xffdc2f54, 0xffdd3051, 0xffdd314e, 0xffde324a, 0xffde3347, 0xffdf3444, 0xffdf3541, 0xffdf363d, 0xffe0373a, 0xffe03837, 0xffe03933, 0xffe13a30, 0xffe23b2d, 0xffe23c2a, 0xffe33d26, 0xffe33e23, 0xffe43f20, 0xffe4411d, 0xffe4421c, 0xffe5431b, 0xffe54419, 0xffe54518, 0xffe64616, 0xffe74715, 0xffe74814, 0xffe74913, 0xffe84a12, 0xffe84c10, 0xffe84c0f, 0xffe94d0e, 0xffe94d0d, 0xffea4e0c, 0xffea4f0c, 0xffeb500b, 0xffeb510a, 0xffeb520a, 0xffeb5309, 0xffec5409, 0xffec5608, 0xffec5708, 0xffec5808, 0xffed5907, 0xffed5a07, 0xffed5b06, 0xffee5c06, 0xffee5c05, 0xffee5d05, 0xffee5e05, 0xffef5f04, 0xffef6004, 0xffef6104, 0xffef6204, 0xfff06303, 0xfff06403, 0xfff06503, 0xfff16603, 0xfff16603, 0xfff16703, 0xfff16803, 0xfff16902, 0xfff16a02, 0xfff16b02, 0xfff16b02, 0xfff26c01, 0xfff26d01, 0xfff26e01, 0xfff36f01, 0xfff37001, 0xfff37101, 0xfff37201, 0xfff47300, 0xfff47400, 0xfff47500, 0xfff47600, 0xfff47700, 0xfff47800, 0xfff47a00, 0xfff57b00, 0xfff57c00, 0xfff57e00, 0xfff57f00, 0xfff68000, 0xfff68100, 0xfff68200, 0xfff78300, 0xfff78400, 0xfff78500, 0xfff78600, 0xfff88700, 0xfff88800, 0xfff88800, 0xfff88900, 0xfff88a00, 0xfff88b00, 0xfff88c00, 0xfff98d00, 0xfff98d00, 0xfff98e00, 0xfff98f00, 0xfff99000, 0xfff99100, 0xfff99200, 0xfff99300, 0xfffa9400, 0xfffa9500, 0xfffa9600, 0xfffb9800, 0xfffb9900, 0xfffb9a00, 0xfffb9c00, 0xfffc9d00, 0xfffc9f00, 0xfffca000, 0xfffca100, 0xfffda200, 0xfffda300, 0xfffda400, 0xfffda600, 0xfffda700, 0xfffda800, 0xfffdaa00, 0xfffdab00, 0xfffdac00, 0xfffdad00, 0xfffdae00, 0xfffeaf00, 0xfffeb000, 0xfffeb100, 0xfffeb200, 0xfffeb300, 0xfffeb400, 0xfffeb500, 0xfffeb600, 0xfffeb800, 0xfffeb900, 0xfffeb900, 0xfffeba00, 0xfffebb00, 0xfffebc00, 0xfffebd00, 0xfffebe00, 0xfffec000, 0xfffec100, 0xfffec200, 0xfffec300, 0xfffec400, 0xfffec500, 0xfffec600, 0xfffec700, 0xfffec800, 0xfffec901, 0xfffeca01, 0xfffeca01, 0xfffecb01, 0xfffecc02, 0xfffecd02, 0xfffece03, 0xfffecf04, 0xfffecf04, 0xfffed005, 0xfffed106, 0xfffed308, 0xfffed409, 0xfffed50a, 0xfffed60a, 0xfffed70b, 0xfffed80c, 0xfffed90d, 0xffffda0e, 0xffffda0e, 0xffffdb10, 0xffffdc12, 0xffffdc14, 0xffffdd16, 0xffffde19, 0xffffde1b, 0xffffdf1e, 0xffffe020, 0xffffe122, 0xffffe224, 0xffffe226, 0xffffe328, 0xffffe42b, 0xffffe42e, 0xffffe531, 0xffffe635, 0xffffe638, 0xffffe73c, 0xffffe83f, 0xffffe943, 0xffffea46, 0xffffeb49, 0xffffeb4d, 0xffffec50, 0xffffed54, 0xffffee57, 0xffffee5b, 0xffffee5f, 0xffffef63, 0xffffef67, 0xfffff06a, 0xfffff06e, 0xfffff172, 0xfffff177, 0xfffff17b, 0xfffff280, 0xfffff285, 0xfffff28a, 0xfffff38e, 0xfffff492, 0xfffff496, 0xfffff49a, 0xfffff59e, 0xfffff5a2, 0xfffff5a6, 0xfffff6aa, 0xfffff6af, 0xfffff7b3, 0xfffff7b6, 0xfffff8ba, 0xfffff8bd, 0xfffff8c1, 0xfffff8c4, 0xfffff9c7, 0xfffff9ca, 0xfffff9cd, 0xfffffad1, 0xfffffad4, 0xfffffbd8, 0xfffffcdb, 0xfffffcdf, 0xfffffde2, 0xfffffde5, 0xfffffde8, 0xfffffeeb, 0xfffffeee, 0xfffffef1, 0xfffffef4, 0xfffffff6 };
static uint32_t colors_phase[] = { 0xff000cff, 0xff0010ff, 0xff0014ff, 0xff0019ff, 0xff001dff, 0xff0021ff, 0xff0025ff, 0xff0029ff, 0xff002dff, 0xff0032ff, 0xff0036ff, 0xff003aff, 0xff003eff, 0xff0042ff, 0xff0046ff, 0xff004bff, 0xff004fff, 0xff0053ff, 0xff0057ff, 0xff005bff, 0xff005fff, 0xff0063ff, 0xff0068ff, 0xff006cff, 0xff0070ff, 0xff0074ff, 0xff0078ff, 0xff007cff, 0xff0081ff, 0xff0085ff, 0xff0089ff, 0xff008dff, 0xff0091ff, 0xff0095ff, 0xff0099ff, 0xff009eff, 0xff00a2ff, 0xff00a6ff, 0xff00aaff, 0xff00aeff, 0xff00b2ff, 0xff00b7ff, 0xff00bbff, 0xff00bfff, 0xff00c3ff, 0xff00c7ff, 0xff00cbff, 0xff00cfff, 0xff00d4ff, 0xff00d8ff, 0xff00dcff, 0xff00e0ff, 0xff00e4ff, 0xff00e8ff, 0xff00edff, 0xff00f1ff, 0xff00f5ff, 0xff00f9ff, 0xff00fdff, 0xff00fffd, 0xff00fff8, 0xff00fff4, 0xff00ffef, 0xff00ffeb, 0xff00ffe6, 0xff00ffe2, 0xff00ffde, 0xff00ffd9, 0xff00ffd5, 0xff00ffd0, 0xff00ffcc, 0xff00ffc8, 0xff00ffc3, 0xff00ffbf, 0xff00ffba, 0xff00ffb6, 0xff00ffb1, 0xff00ffad, 0xff00ffa9, 0xff00ffa4, 0xff00ffa0, 0xff00ff9b, 0xff00ff97, 0xff00ff93, 0xff00ff8e, 0xff00ff8a, 0xff00ff85, 0xff00ff81, 0xff00ff7c, 0xff00ff78, 0xff00ff74, 0xff00ff6f, 0xff00ff6b, 0xff00ff66, 0xff00ff62, 0xff00ff5e, 0xff00ff59, 0xff00ff55, 0xff00ff50, 0xff00ff4c, 0xff00ff48, 0xff00ff43, 0xff00ff3f, 0xff00ff3a, 0xff00ff36, 0xff00ff31, 0xff00ff2d, 0xff00ff29, 0xff00ff24, 0xff00ff20, 0xff00ff1b, 0xff00ff17, 0xff00ff13, 0xff00ff0e, 0xff00ff0a, 0xff00ff05, 0xff00ff01, 0xff03ff00, 0xff07ff00, 0xff0cff00, 0xff10ff00, 0xff14ff00, 0xff18ff00, 0xff1cff00, 0xff20ff00, 0xff25ff00, 0xff29ff00, 0xff2dff00, 0xff31ff00, 0xff35ff00, 0xff39ff00, 0xff3eff00, 0xff42ff00, 0xff46ff00, 0xff4aff00, 0xff4eff00, 0xff52ff00, 0xff56ff00, 0xff5bff00, 0xff5fff00, 0xff63ff00, 0xff67ff00, 0xff6bff00, 0xff6fff00, 0xff74ff00, 0xff78ff00, 0xff7cff00, 0xff80ff00, 0xff84ff00, 0xff88ff00, 0xff8cff00, 0xff91ff00, 0xff95ff00, 0xff99ff00, 0xff9dff00, 0xffa1ff00, 0xffa5ff00, 0xffaaff00, 0xffaeff00, 0xffb2ff00, 0xffb6ff00, 0xffbaff00, 0xffbeff00, 0xffc2ff00, 0xffc7ff00, 0xffcbff00, 0xffcfff00, 0xffd3ff00, 0xffd7ff00, 0xffdbff00, 0xffe0ff00, 0xffe4ff00, 0xffe8ff00, 0xffecff00, 0xfff0ff00, 0xfff4ff00, 0xfff8ff00, 0xfffdff00, 0xfffffd00, 0xfffff900, 0xfffff500, 0xfffff100, 0xffffed00, 0xffffe800, 0xffffe400, 0xffffe000, 0xffffdc00, 0xffffd800, 0xffffd400, 0xffffd000, 0xffffcb00, 0xffffc700, 0xffffc300, 0xffffbf00, 0xffffbb00, 0xffffb700, 0xffffb200, 0xffffae00, 0xffffaa00, 0xffffa600, 0xffffa200, 0xffff9e00, 0xffff9900, 0xffff9500, 0xffff9100, 0xffff8d00, 0xffff8900, 0xffff8500, 0xffff8100, 0xffff7c00, 0xffff7800, 0xffff7400, 0xffff7000, 0xffff6c00, 0xffff6800, 0xffff6300, 0xffff5f00, 0xffff5b00, 0xffff5700, 0xffff5300, 0xffff4f00, 0xffff4b00, 0xffff4600, 0xffff4200, 0xffff3e00, 0xffff3a00, 0xffff3600, 0xffff3200, 0xffff2d00, 0xffff2900, 0xffff2500, 0xffff2100, 0xffff1d00, 0xffff1900, 0xffff1500, 0xffff1000, 0xffff0c00, 0xffff0800, 0xffff0400, 0xffff0000, 0xffff0005, 0xffff0009, 0xffff000e, 0xffff0012, 0xffff0016, 0xffff001b, 0xffff001f, 0xffff0024, 0xffff0028, 0xffff002c, 0xffff0031, 0xffff0035, 0xffff003a, 0xffff003e, 0xffff0042, 0xffff0047, 0xffff004b, 0xffff0050, 0xffff0054, 0xffff0059, 0xffff005d, 0xffff0061, 0xffff0066, 0xffff006a, 0xffff006f, 0xffff0073, 0xffff0077, 0xffff007c, 0xffff0080, 0xffff0085, 0xffff0089, 0xffff008e, 0xffff0092, 0xffff0096, 0xffff009b, 0xffff009f, 0xffff00a4, 0xffff00a8, 0xffff00ac, 0xffff00b1, 0xffff00b5, 0xffff00ba, 0xffff00be, 0xffff00c3, 0xffff00c7, 0xffff00cb, 0xffff00d0, 0xffff00d4, 0xffff00d9, 0xffff00dd, 0xffff00e1, 0xffff00e6, 0xffff00ea, 0xffff00ef, 0xffff00f3, 0xffff00f7, 0xffff00fc, 0xfffe00ff, 0xfffa00ff, 0xfff500ff, 0xfff100ff, 0xffed00ff, 0xffe900ff, 0xffe500ff, 0xffe100ff, 0xffdd00ff, 0xffd800ff, 0xffd400ff, 0xffd000ff, 0xffcc00ff, 0xffc800ff, 0xffc400ff, 0xffbf00ff, 0xffbb00ff, 0xffb700ff, 0xffb300ff, 0xffaf00ff, 0xffab00ff, 0xffa600ff, 0xffa200ff, 0xff9e00ff, 0xff9a00ff, 0xff9600ff, 0xff9200ff, 0xff8e00ff, 0xff8900ff, 0xff8500ff, 0xff8100ff, 0xff7d00ff, 0xff7900ff, 0xff7500ff, 0xff7000ff, 0xff6c00ff, 0xff6800ff, 0xff6400ff, 0xff6000ff, 0xff5c00ff, 0xff5800ff, 0xff5300ff, 0xff4f00ff, 0xff4b00ff, 0xff4700ff, 0xff4300ff, 0xff3f00ff, 0xff3a00ff, 0xff3600ff, 0xff3200ff, 0xff2e00ff, 0xff2a00ff, 0xff2600ff, 0xff2200ff, 0xff1d00ff, 0xff1900ff, 0xff1500ff, 0xff1100ff, 0xff0d00ff, 0xff0900ff, 0xff000000, };
static uint32_t colors_overlay[256];
static uint32_t colors_shadow[256];

struct palette pal_heatmap = {
    .rgba = colors_heatmap,
    .count = sizeof(colors_heatmap) / sizeof(colors_heatmap[0])
};

struct palette pal_overlay = {
    .rgba = colors_overlay,
    .count = sizeof(colors_overlay) / sizeof(colors_overlay[0])
};

struct palette pal_shadow = {
    .rgba = colors_shadow,
    .count = sizeof(colors_shadow) / sizeof(colors_shadow[0])
};

struct palette pal_phase = {
    .rgba = colors_phase,
    .count = sizeof(colors_phase) / sizeof(colors_phase[0])
};


struct img *img_new(int w, int h)
{
    struct img *img = malloc(sizeof(*img));
    img->w = w;
    img->h = h;
    img->pix = calloc(w * h, sizeof(double));
    return img;
}


void run_script(struct ctx *ctx, bool on)
{
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %d", ctx->script, on);
    int r = system(cmd);
    if(r != 0) {
        fprintf(stderr, "error: script failed\n");
        exit(1);
    }
}


void write_img(struct ctx *ctx, struct img *img, struct palette *pal, int dst_x, int dst_y)
{
    int y, x;

    double vmin = 1.0e20;
    double vmax = 0.0;

    for(int i=0; i<img->w * img->h; i++) {
        double v = img->pix[i];
        if(v != 0) {
            if(v < vmin) vmin = v;
            if(v > vmax) vmax = v;
        }
    }

    if(vmax <= vmin) vmax = vmin + 1;

    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(
            0, img->w, img->h, 32, SDL_PIXELFORMAT_BGRA32);

    SDL_LockSurface(surf);
    uint32_t *pixels = surf->pixels;

    for(y=img->h-1; y>=0; y--) {
        for(x=img->w-1; x>=0; x--) {
            int v = pal->count * (img->pix[y * img->w + x] - vmin) / (vmax - vmin);
            if(v < 0) v = 0;
            if(v > pal->count-1) v = pal->count-1;
            pixels[y * img->w + x] = pal->rgba[v];
        }
    }

    SDL_UnlockSurface(surf);
    SDL_Rect dst = { dst_x + 2, dst_y + 2, ctx->win_w * 0.5 - 4, ctx->win_h * 0.5 - 4 };
    SDL_BlitScaled(surf, NULL, SDL_GetWindowSurface(ctx->win), &dst);
    SDL_FreeSurface(surf);

}


void on_frame(struct ctx *ctx, struct img *img)
{
    double t = (double)ctx->n / (double)ctx->n_cycle;
    double fact_s = sin(t * 2 * M_PI);
    double fact_c = cos(t * 2 * M_PI);

    for(int i=0; i<img->w * img->h; i++) {
        ctx->img_re->pix[i] += img->pix[i] * fact_c;
        ctx->img_im->pix[i] += img->pix[i] * fact_s;
    }

    if(ctx->n == ctx->n_cycle / 2) {
        run_script(ctx, true);
    }

    if(ctx->n == ctx->n_cycle) {
        run_script(ctx, false);

        for(int i=0; i<img->w * img->h; i++) {
            ctx->img_abs->pix[i] = hypot(ctx->img_im->pix[i], ctx->img_re->pix[i]);
            ctx->img_pha->pix[i] = atan2(ctx->img_im->pix[i], ctx->img_re->pix[i]);
        }

        ctx->n = 0 ;
    }
    
    write_img(ctx, img, &pal_heatmap, 0, 0);
    write_img(ctx, img, &pal_shadow, ctx->win_w * 0.5, 0);
    write_img(ctx, ctx->img_abs, &pal_overlay, ctx->win_w * 0.5, ctx->win_h * 0.0);
    write_img(ctx, ctx->img_abs, &pal_heatmap, ctx->win_w * 0.0, ctx->win_h * 0.5);
    write_img(ctx, ctx->img_pha, &pal_phase, ctx->win_w * 0.5, ctx->win_h * 0.5);
    SDL_UpdateWindowSurface(ctx->win);
}


void init(struct ctx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    for(size_t i=0; i<256; i++) {
        pal_overlay.rgba[i] = (i << 24) | 0x00ff0000;
        pal_shadow.rgba[i] = 0xff000000 | ((i/2) << 16) | ((i/2) << 8) | (i/2);
    }

    ctx->n_cycle = 500;
    ctx->running = false;
    ctx->flir_w = 80;
    ctx->flir_h = 60;
    ctx->img_raw = img_new(ctx->flir_w, ctx->flir_h);
    ctx->img_re = img_new(ctx->flir_w, ctx->flir_h);
    ctx->img_im = img_new(ctx->flir_w, ctx->flir_h);
    ctx->img_abs = img_new(ctx->flir_w, ctx->flir_h);
    ctx->img_pha = img_new(ctx->flir_w, ctx->flir_h);
    ctx->script = "./toggle";

    ctx->win_w = 1024;
    ctx->win_h = 768;

    ctx->win = SDL_CreateWindow("flir",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            ctx->win_w, ctx->win_h,
            SDL_WINDOW_RESIZABLE);

    ctx->handle = hid_open(0x1234, 0x5678, NULL);
}


void poll_sdl(struct ctx *ctx)
{
    SDL_Event ev;
    while(SDL_PollEvent(&ev)) {
        if(ev.type == SDL_QUIT) {
            exit(0);
        }

        if(ev.type == SDL_KEYDOWN) {
            if(ev.key.keysym.sym == SDLK_ESCAPE) {
                exit(0);
            }
            if(ev.key.keysym.sym == SDLK_q) {
                exit(0);
            }
        }

        if(ev.type == SDL_WINDOWEVENT) {
            if(ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                ctx->win_w = ev.window.data1;
                ctx->win_h = ev.window.data2;
            }
        }
    }
}


void poll_console(struct ctx *ctx)
{
    struct pollfd pfd = { .fd = 0, .events = POLLIN };
    int r = poll(&pfd, 1, 0);
    if(r == 1) {
        uint8_t cmd[1];
        r = read(0, cmd, sizeof(cmd));
        hid_write(ctx->handle, cmd, r);
    }
}


void poll_flir(struct ctx *ctx)
{
    uint8_t buf[65];

    int res = hid_read(ctx->handle, buf, 64);
    if (res < 0) {
        return;
    }

    if(buf[0] == 0xff) {

        fprintf(stderr, "%s", buf+1);

    } else {

        struct bitreader br;
        br_init(&br, buf+1, sizeof(buf)-1);

        int y = buf[0];

        // decode one line

        uint16_t v = 0;
        for(int x=0; x<ctx->flir_w; x++) {
            int32_t w;
            br_golomb(&br, &w, 2);
            v += w;
            ctx->img_raw->pix[y*ctx->flir_w+x] = v;
        }

        // frame complete

        if(y == ctx->flir_h-1) {
            if(!ctx->running) {
                // skip first frame as it may be incomplete
                memset(ctx->img_raw->pix, 0, sizeof(double) * ctx->flir_w * ctx->flir_h);
                ctx->running = true;
            } else {
                on_frame(ctx, ctx->img_raw);
            }
            ctx->n ++;
        }
    }
}


void run(struct ctx *ctx)
{
    run_script(ctx, false);

    for(;;) {
        poll_sdl(ctx);
        poll_console(ctx);
        poll_flir(ctx);
    }
}


void usage(void)
{
    fprintf(stderr, "usage: flir [options]\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h          print this message\n");
    fprintf(stderr, "  -n <n>      number of frames for integration\n");
    fprintf(stderr, "  -s <script> path to toggle script\n");
}


int main(int argc, char* argv[])
{
    struct ctx ctx;
    init(&ctx);

    int c;
    while((c = getopt(argc, argv, "hn:s:")) != -1) {
        switch(c) {
            case 'h':
                usage();
                exit(0);
            case 'n':
                ctx.n_cycle = atoi(optarg);
                break;
            case 's':
                ctx.script = optarg;
                break;
        }
    }

    for(;;) {
        run(&ctx);
    }

    return 0;
}

// vi: ts=4 sw=4 et
