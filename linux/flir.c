
// https://www-old.mpi-halle.mpg.de/mpi/publi/pdf/540_02.pdf

#include <stdio.h>
#include <assert.h>
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
#include <SDL_ttf.h>

#include "hidapi.h"
#include "golomb.h"

struct img {
    int w, h;
    double *pix;
};


struct ctx {
    SDL_Window *win;
    SDL_Renderer *rend;
    TTF_Font *font;
    int win_w, win_h;
    bool stimulus;
    double t;
    int n_cycle;
    double decay;
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
static uint32_t colors_phase[] = { 0xff000686, 0xff000886, 0xff000a86, 0xff000d86, 0xff000f86, 0xff001186, 0xff001386, 0xff001586, 0xff001886, 0xff001a86, 0xff001c86, 0xff001e86, 0xff002086, 0xff002386, 0xff002586, 0xff002786, 0xff002986, 0xff002b86, 0xff002e86, 0xff003086, 0xff003286, 0xff003486, 0xff003686, 0xff003986, 0xff003b86, 0xff003d86, 0xff003f86, 0xff004186, 0xff004486, 0xff004686, 0xff004886, 0xff004a86, 0xff004c86, 0xff004e86, 0xff005086, 0xff005386, 0xff005586, 0xff005786, 0xff005986, 0xff005b86, 0xff005d86, 0xff006086, 0xff006286, 0xff006486, 0xff006686, 0xff006886, 0xff006a86, 0xff006c86, 0xff006f86, 0xff007186, 0xff007386, 0xff007586, 0xff007786, 0xff007986, 0xff007c86, 0xff007e86, 0xff008086, 0xff008286, 0xff008486, 0xff008684, 0xff008682, 0xff008680, 0xff00867d, 0xff00867b, 0xff008678, 0xff008676, 0xff008674, 0xff008672, 0xff008670, 0xff00866d, 0xff00866b, 0xff008669, 0xff008666, 0xff008664, 0xff008661, 0xff00865f, 0xff00865d, 0xff00865b, 0xff008658, 0xff008656, 0xff008654, 0xff008651, 0xff00864f, 0xff00864d, 0xff00864a, 0xff008648, 0xff008646, 0xff008644, 0xff008641, 0xff00863f, 0xff00863d, 0xff00863a, 0xff008638, 0xff008635, 0xff008633, 0xff008631, 0xff00862f, 0xff00862d, 0xff00862a, 0xff008628, 0xff008626, 0xff008623, 0xff008621, 0xff00861e, 0xff00861c, 0xff00861a, 0xff008618, 0xff008615, 0xff008613, 0xff008611, 0xff00860e, 0xff00860c, 0xff00860a, 0xff008607, 0xff008605, 0xff008603, 0xff008601, 0xff028600, 0xff048600, 0xff068600, 0xff088600, 0xff0a8600, 0xff0d8600, 0xff0f8600, 0xff118600, 0xff138600, 0xff158600, 0xff188600, 0xff1a8600, 0xff1c8600, 0xff1e8600, 0xff208600, 0xff238600, 0xff258600, 0xff278600, 0xff298600, 0xff2b8600, 0xff2d8600, 0xff308600, 0xff328600, 0xff348600, 0xff368600, 0xff388600, 0xff3a8600, 0xff3d8600, 0xff3f8600, 0xff418600, 0xff438600, 0xff458600, 0xff478600, 0xff498600, 0xff4c8600, 0xff4e8600, 0xff508600, 0xff528600, 0xff548600, 0xff568600, 0xff598600, 0xff5b8600, 0xff5d8600, 0xff5f8600, 0xff618600, 0xff638600, 0xff668600, 0xff688600, 0xff6a8600, 0xff6c8600, 0xff6e8600, 0xff718600, 0xff738600, 0xff758600, 0xff778600, 0xff798600, 0xff7c8600, 0xff7e8600, 0xff808600, 0xff828600, 0xff848600, 0xff868400, 0xff868200, 0xff868000, 0xff867e00, 0xff867c00, 0xff867900, 0xff867700, 0xff867500, 0xff867300, 0xff867100, 0xff866f00, 0xff866d00, 0xff866a00, 0xff866800, 0xff866600, 0xff866400, 0xff866200, 0xff866000, 0xff865d00, 0xff865b00, 0xff865900, 0xff865700, 0xff865500, 0xff865300, 0xff865000, 0xff864e00, 0xff864c00, 0xff864a00, 0xff864800, 0xff864600, 0xff864400, 0xff864100, 0xff863f00, 0xff863d00, 0xff863b00, 0xff863900, 0xff863600, 0xff863400, 0xff863200, 0xff863000, 0xff862e00, 0xff862b00, 0xff862900, 0xff862700, 0xff862500, 0xff862300, 0xff862000, 0xff861e00, 0xff861c00, 0xff861a00, 0xff861800, 0xff861500, 0xff861300, 0xff861100, 0xff860f00, 0xff860d00, 0xff860b00, 0xff860800, 0xff860600, 0xff860400, 0xff860200, 0xff860000, 0xff860003, 0xff860005, 0xff860007, 0xff860009, 0xff86000c, 0xff86000e, 0xff860010, 0xff860013, 0xff860015, 0xff860017, 0xff86001a, 0xff86001c, 0xff86001e, 0xff860020, 0xff860023, 0xff860025, 0xff860027, 0xff86002a, 0xff86002c, 0xff86002f, 0xff860031, 0xff860033, 0xff860035, 0xff860038, 0xff86003a, 0xff86003c, 0xff86003e, 0xff860041, 0xff860043, 0xff860046, 0xff860048, 0xff86004a, 0xff86004c, 0xff86004f, 0xff860051, 0xff860053, 0xff860056, 0xff860058, 0xff86005a, 0xff86005d, 0xff86005f, 0xff860061, 0xff860063, 0xff860066, 0xff860068, 0xff86006a, 0xff86006d, 0xff86006f, 0xff860072, 0xff860074, 0xff860076, 0xff860078, 0xff86007b, 0xff86007d, 0xff86007f, 0xff860081, 0xff860084, 0xff850086, 0xff830086, 0xff800086, 0xff7e0086, 0xff7c0086, 0xff7a0086, 0xff780086, 0xff760086, 0xff740086, 0xff710086, 0xff6f0086, 0xff6d0086, 0xff6b0086, 0xff690086, 0xff670086, 0xff640086, 0xff620086, 0xff600086, 0xff5e0086, 0xff5c0086, 0xff5a0086, 0xff570086, 0xff550086, 0xff530086, 0xff510086, 0xff4f0086, 0xff4c0086, 0xff4a0086, 0xff480086, 0xff460086, 0xff440086, 0xff410086, 0xff3f0086, 0xff3d0086, 0xff3b0086, 0xff390086, 0xff360086, 0xff340086, 0xff320086, 0xff300086, 0xff2e0086, 0xff2b0086, 0xff290086, 0xff270086, 0xff250086, 0xff230086, 0xff210086, 0xff1e0086, 0xff1c0086, 0xff1a0086, 0xff180086, 0xff160086, 0xff140086, 0xff120086, 0xff0f0086, 0xff0d0086, 0xff0b0086, 0xff090086, 0xff070086, 0xff050086, };

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


void set_stimulus(struct ctx *ctx, bool on)
{
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %d", ctx->script, on);
    int r = system(cmd);
    if(r != 0) {
        fprintf(stderr, "error: script failed\n");
        exit(1);
    }
    ctx->stimulus = on;
}


void draw_text(struct ctx *ctx, const char *text, int x, int y)
{
    SDL_Color fg = { 255, 255, 255 };
    SDL_Surface * surface = TTF_RenderUTF8_Blended(ctx->font, text, fg);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ctx->rend, surface);
    SDL_FreeSurface(surface);
    int w, h;
    SDL_QueryTexture(tex, NULL, NULL, &w, &h);
    SDL_Rect dst = { x, y, w, h };
    SDL_RenderCopy(ctx->rend, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
}


void blit_img(struct ctx *ctx, const char *label, struct img *img, struct palette *pal, double dst_x, double dst_y)
{
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

    SDL_Texture *tex = SDL_CreateTexture( ctx->rend, 
            SDL_PIXELFORMAT_ARGB8888, 
            SDL_TEXTUREACCESS_STREAMING, img->w, img->h);

    uint32_t *pixels;
    int pitch;
    SDL_LockTexture(tex, NULL, (void **)&pixels, &pitch);
    for(int i=0; i<img->w * img->h; i++) {
        int v = pal->count * (img->pix[i] - vmin) / (vmax - vmin);
        if(v < 0) v = 0;
        if(v > pal->count-1) v = pal->count-1;
        pixels[i] = pal->rgba[v];
        
    }
    SDL_UnlockTexture(tex);

    int x = (ctx->win_w) * dst_x + 2;
    int y = (ctx->win_h - 20) * dst_y + 2 + 20;
    int w = (ctx->win_w) * 0.5 - 4;
    int h = (ctx->win_h - 20) * 0.5 - 4;
    
    SDL_Rect dst = { x, y, w, h };
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(ctx->rend, tex, NULL, &dst);
    SDL_DestroyTexture(tex);

    if(label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s %.2g", label, vmax-vmin);
        draw_text(ctx, buf, x + 5, y + 5);
    }

}


void on_frame(struct ctx *ctx, struct img *img)
{
    double t = (double)ctx->n / (double)ctx->n_cycle;
    double f_re = cos(t * 2.0 * M_PI);
    double f_im = sin(t * 2.0 * M_PI);

    for(int i=0; i<img->w * img->h; i++) {
        ctx->img_re->pix[i] += img->pix[i] * f_re;
        ctx->img_im->pix[i] += img->pix[i] * f_im;
    }

    ctx->n ++;

    if(ctx->n == ctx->n_cycle / 2) {
        // half cycle toggle stimulus on
        set_stimulus(ctx, false);
    }

    if(ctx->n == ctx->n_cycle) {
        // end of cycle toggle stimulus off
        set_stimulus(ctx, true);

        // calculate magnitude and phase
        for(int i=0; i<img->w * img->h; i++) {
            double im = ctx->img_im->pix[i] / ctx->n_cycle;
            double re = ctx->img_re->pix[i] / ctx->n_cycle;
            ctx->img_abs->pix[i] = hypot(im, re);
            ctx->img_pha->pix[i] = atan2(im, re);
        }

        // decay integration
        for(int i=0; i<img->w * img->h; i++) {
            ctx->img_re->pix[i] *= ctx->decay;
            ctx->img_im->pix[i] *= ctx->decay;
        }

        ctx->n = 0 ;
    }

    // render output

    SDL_RenderFillRect(ctx->rend, NULL);
   
    blit_img(ctx, "raw",  img, &pal_heatmap,           0.0, 0.0);
    blit_img(ctx, NULL,   img, &pal_shadow,            0.5, 0.0);
    blit_img(ctx, NULL,   ctx->img_abs, &pal_overlay,  0.5, 0.0);
    blit_img(ctx, "|T|",  ctx->img_abs, &pal_heatmap,  0.0, 0.5);
    blit_img(ctx, "φT",   ctx->img_pha, &pal_phase,    0.5, 0.5);

    char buf[256];
    snprintf(buf, sizeof(buf), "out=%d n=%d/%d cycle=%d",
            ctx->stimulus, (ctx->n % ctx->n_cycle), ctx->n_cycle, ctx->n / ctx->n_cycle);
    draw_text(ctx, buf, 2, 2);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderPresent(ctx->rend);
    SDL_UpdateWindowSurface(ctx->win);
}


void init(struct ctx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    for(size_t i=0; i<256; i++) {
        pal_overlay.rgba[i] = (i << 24) | 0x00ff0000;
        pal_shadow.rgba[i] = 0xff000000 | ((i/2) << 16) | ((i/2) << 8) | (i/2);
    }

    ctx->n_cycle = 100;
    ctx->decay = 1.0;
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

    ctx->rend = SDL_CreateRenderer(ctx->win, -1, SDL_RENDERER_ACCELERATED);
	
    TTF_Init();
	ctx->font = TTF_OpenFont("font.ttf", 13);
	assert(ctx->font);

    ctx->handle = hid_open(0x1234, 0x5678, NULL);
}


void reset(struct ctx *ctx, int n_cycle)
{
    set_stimulus(ctx, true);
                
    memset(ctx->img_re->pix, 0, sizeof(double) * ctx->flir_w * ctx->flir_h);
    memset(ctx->img_im->pix, 0, sizeof(double) * ctx->flir_w * ctx->flir_h);
    memset(ctx->img_abs->pix, 0, sizeof(double) * ctx->flir_w * ctx->flir_h);
    memset(ctx->img_pha->pix, 0, sizeof(double) * ctx->flir_w * ctx->flir_h);

    ctx->n_cycle = n_cycle;
    ctx->n = 0;
    ctx->running = false;

    if(ctx->n_cycle < 10) {
        ctx->n_cycle = 10;
    }
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
                set_stimulus(ctx, false);
                exit(0);
            }
            if(ev.key.keysym.sym == SDLK_r) {
                reset(ctx, ctx->n_cycle);
            }
            if(ev.key.keysym.sym == SDLK_RIGHTBRACKET) {
                reset(ctx, ctx->n_cycle + 10);
            }
            if(ev.key.keysym.sym == SDLK_LEFTBRACKET) {
                reset(ctx, ctx->n_cycle - 10);
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
        }
    }
}


void run(struct ctx *ctx)
{
    // stop any previous stimulus before starting
    set_stimulus(ctx, false);
    set_stimulus(ctx, true);

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
    while((c = getopt(argc, argv, "hd:n:s:")) != -1) {
        switch(c) {
            case 'd':
                ctx.decay = atof(optarg);
                break;
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
