#ifndef NOVA_H
#define NOVA_H

#include <stdint.h>
#include <stddef.h>

/* NOVA-8 fantasy console core definitions.
   little machine, big dreams. */

#define NOVA_MAGIC      0x41564F4Eu  /* 'NOVA' LE */
#define NOVA_VERSION    0x0002

#define FOURCC(a,b,c,d) ((uint32_t)(a) | ((uint32_t)(b)<<8) | ((uint32_t)(c)<<16) | ((uint32_t)(d)<<24))

#define TAG_CODE  FOURCC('C','O','D','E')
#define TAG_DATA  FOURCC('D','A','T','A')
#define TAG_SPRT  FOURCC('S','P','R','T')
#define TAG_MAP   FOURCC('M','A','P',' ')
#define TAG_PAL   FOURCC('P','A','L',' ')
#define TAG_SND   FOURCC('S','N','D',' ')
#define TAG_PATN  FOURCC('P','A','T','N')
#define TAG_FONT  FOURCC('F','O','N','T')
#define TAG_META  FOURCC('M','E','T','A')
#define TAG_SAVE  FOURCC('S','A','V','E')

/* machine sizing */
#define NOVA_RAM_SIZE     0x10000     /* 64 KiB main RAM */
#define NOVA_NUM_BANKS    16
#define NOVA_NUM_REGS     16
#define NOVA_FB_W         160
#define NOVA_FB_H         144
#define NOVA_FB_SIZE      (NOVA_FB_W * NOVA_FB_H)
#define NOVA_MAX_VOICES   8
#define NOVA_MAX_SPRITES  64

#define NOVA_STACK_INIT   64
#define NOVA_FRAME_INIT   16
#define NOVA_STREAM_DEPTH 8

/* opcodes -- one byte each, operands follow inline */
enum {
    OP_NOP   = 0x00,
    OP_HALT  = 0x01,
    OP_PUSH  = 0x02,   /* imm32 */
    OP_PUSHB = 0x03,   /* imm8  */
    OP_POP   = 0x04,
    OP_DUP   = 0x05,
    OP_SWAP  = 0x06,
    OP_ADD   = 0x07,
    OP_SUB   = 0x08,
    OP_MUL   = 0x09,
    OP_DIV   = 0x0A,
    OP_MOD   = 0x0B,
    OP_AND   = 0x0C,
    OP_OR    = 0x0D,
    OP_XOR   = 0x0E,
    OP_SHL   = 0x0F,
    OP_SHR   = 0x10,
    OP_NEG   = 0x11,
    OP_NOT   = 0x12,
    OP_CMP   = 0x13,
    OP_ACCUM = 0x14,   /* fold top N into accumulator slot */

    OP_LD    = 0x20,   /* load reg[imm8] */
    OP_ST    = 0x21,   /* store reg[imm8] */
    OP_LDM   = 0x22,   /* load ram[addr] (addr from stack) */
    OP_STM   = 0x23,   /* store ram[addr] */
    OP_BANK  = 0x24,   /* select bank imm8 */
    OP_DMA   = 0x25,   /* block copy: len,src,dst on stack */
    OP_LREF  = 0x26,   /* box ref to local imm8 -> handle */
    OP_LDREF = 0x27,   /* deref handle on stack */
    OP_EXEC  = 0x28,   /* switch pc into RAM exec window */

    OP_JMP   = 0x30,   /* imm32 target */
    OP_JZ    = 0x31,
    OP_JNZ   = 0x32,
    OP_CALL  = 0x33,   /* imm32 target, imm8 nlocals */
    OP_RET   = 0x34,
    OP_SWITCH= 0x35,   /* jump table in DATA: idx on stack */

    OP_SPUSH = 0x40,   /* push data-stream cursor */
    OP_SPOP  = 0x41,   /* pop data-stream cursor */
    OP_SREAD = 0x42,   /* read byte at cursor, advance */
    OP_SSEEK = 0x43,   /* seek cursor by signed delta */

    OP_SPR   = 0x50,   /* draw sprite: id,x,y on stack */
    OP_TILE  = 0x51,   /* draw tilemap region */
    OP_PALSET= 0x52,   /* set palette bank */
    OP_PALCYC= 0x53,   /* start palette cycle: lo,hi on stack */
    OP_CAM   = 0x54,   /* set camera dx,dy */
    OP_TEXT  = 0x55,   /* draw text: str-addr,x,y */

    OP_VOICE = 0x60,   /* bind instrument id to channel */
    OP_NOTE  = 0x61,   /* note on: chan,pitch */
    OP_NOFF  = 0x62,   /* note off: chan */
    OP_PLAY  = 0x63,   /* start pattern sequence at order imm8 */

    OP_SPAWN = 0x70,   /* spawn sprite obj: id,x,y -> handle */
    OP_KILL  = 0x71,   /* despawn sprite handle */
    OP_TICK  = 0x72,   /* advance one machine tick */
    OP_SAVE  = 0x73,   /* snapshot to SAVE region */
    OP_LOAD  = 0x74,   /* restore from SAVE region */
};

/* ---- cartridge ---- */
typedef struct {
    uint32_t tag;
    uint32_t offset;
    uint32_t size;
} nova_chunk;

typedef struct {
    const uint8_t *base;
    size_t         len;
    uint16_t       version;
    uint16_t       flags;
    uint16_t       chunk_cnt;
    uint32_t       entry_pc;
    uint32_t       hdr_crc;
    nova_chunk    *dir;
} nova_cart;

/* ---- memory ---- */
typedef struct {
    uint8_t *ram;
    size_t   ram_size;
    uint32_t bank_base[NOVA_NUM_BANKS];
    uint32_t bank_size[NOVA_NUM_BANKS];
    int      nbanks;
    int      cur_bank;
    uint32_t exec_base;
    uint32_t exec_size;
} nova_mem;

/* ---- vm ---- */
typedef struct nova_frame {
    uint32_t  ret_pc;
    int32_t  *locals;
    uint16_t  nlocals;
} nova_frame;

typedef struct {
    int32_t  *base;     /* points into a frame's heap locals */
    uint16_t  index;
    uint16_t  live;
} nova_handle;

typedef struct {
    uint8_t  *buf;
    uint32_t  size;
    uint32_t  pos;
    int       owned;
} nova_cursor;

typedef struct {
    int32_t   *stack;
    size_t     sp;
    size_t     stack_cap;
    int32_t    reg[NOVA_NUM_REGS];
    nova_frame *frames;
    size_t     fp;
    size_t     frame_cap;
    uint32_t   pc;
    const uint8_t *code;
    uint32_t   code_size;
    nova_handle handles[NOVA_NUM_REGS];
    int        nhandles;
    nova_cursor cursors[NOVA_STREAM_DEPTH];
    int        cur_depth;
    const uint8_t *data;
    uint32_t   data_size;
    int        running;
    int        in_ram_exec;
} nova_vm;

/* ---- graphics ---- */
typedef struct {
    uint8_t  *pixels;     /* w*h, palette indices */
    uint16_t  w, h;
} nova_bank_img;

typedef struct {
    nova_bank_img *banks;
    int            nbanks;
    int            draw_cnt;   /* declared draw-list size */
    uint16_t      *anim;       /* animation frame table */
    int            anim_cnt;
} nova_sprites;

typedef struct {
    uint32_t *entries;   /* RGBA */
    int       ncolors;
    int       nbanks;
    int       cur_bank;
    int       cyc_lo, cyc_hi;
    int       cyc_active;
    int       cyc_phase;
    uint32_t *fade;
    int       fade_cap;
} nova_palette;

typedef struct {
    uint16_t *tiles;
    int       w, h;
    int       tile_w, tile_h;
} nova_tilemap;

typedef struct {
    uint8_t *fb;
    int      cam_x, cam_y;
} nova_gfx;

/* ---- audio ---- */
typedef struct {
    int16_t *pts;        /* envelope points: level pairs */
    int      npts;
    int      loop_start;
    int      loop_end;
    int      sustain;
    int      lfo_depth;
    int      lfo_rate;
} nova_instr;

typedef struct nova_voice {
    int        active;
    int        instr;
    int        pitch;
    int        env_idx;
    int        env_frac;
    int        releasing;
    int        lfo_phase;
    uint32_t   osc_phase;
    int        cur_level;
    struct nova_mod *pending;   /* queued LFO mod */
} nova_voice;

typedef struct nova_mod {
    nova_voice *target;
    int         delta;
    int         when;
} nova_mod;

typedef struct {
    nova_instr *instrs;
    int         ninstrs;
    nova_voice  voices[NOVA_MAX_VOICES];
    nova_mod  **modq;
    int         nmods;
    int         tick;
} nova_synth;

/* ---- tracker ---- */
typedef struct {
    uint8_t *rows;       /* row events: 4 bytes each */
    int      num_rows;
    int      row_stride;
} nova_pattern;

typedef struct {
    nova_pattern *patterns;
    int           npatterns;
    uint8_t      *order;     /* sequence of pattern indices */
    int           order_len;
    int           cur_order;
    int           cur_row;
    int           cur_pat;
    int           speed;
    int           tick_ctr;
    int           playing;
    nova_pattern *cur_buf;   /* active pattern view */
    nova_pattern *next_buf;
} nova_tracker;

/* ---- font ---- */
typedef struct {
    uint8_t  *bitmap;     /* simple glyphs */
    uint8_t  *comps;      /* composite component glyph ids */
    int       ncomps;
    int       w, h;
    int       is_composite;
} nova_glyph;

typedef struct nova_glyph_cache_e {
    int      glyph;
    uint8_t *raster;
    int      size;
    int      pinned;
    struct nova_glyph_cache_e *lru_prev, *lru_next;
} nova_glyph_cache_e;

typedef struct {
    nova_glyph *glyphs;
    int         nglyphs;
    nova_glyph_cache_e *cache;
    int         cache_cap;
    int         cache_used;
    nova_glyph_cache_e *lru_head, *lru_tail;
    uint8_t    *run;          /* glyph run buffer */
    int         run_cap;
    int         run_len;
} nova_font;

/* ---- collision ---- */
typedef struct {
    int      active;
    int      gen;
    int      sprite;
    int      x, y, w, h;
} nova_obj;

typedef struct {
    nova_obj *list[NOVA_MAX_SPRITES];
    int       nobjs;
    int      *freelist;
    int       free_top;
    int16_t  *grid;          /* cell -> obj index */
    int       grid_cols, grid_rows;
    int       iterating;
} nova_world;

/* ---- effects ---- */
typedef struct {
    int vibr_pos, vibr_depth, vibr_speed;
    int porta_target, porta_speed;
    int vol, vol_slide;
    int arp_a, arp_b, arp_pos;
    int active;
} nova_fxstate;

/* ---- machine ---- */
typedef struct {
    nova_cart    cart;
    nova_mem     mem;
    nova_vm      vm;
    nova_gfx     gfx;
    nova_sprites spr;
    nova_palette pal;
    nova_tilemap map;
    nova_synth   synth;
    nova_tracker trk;
    nova_font    font;
    nova_world   world;
    uint8_t     *save_region;
    int          save_size;
    int          ticks_run;
    int16_t     *audio_buf;
    int          audio_cap;
    uint32_t     noise_lfsr;
    nova_fxstate fx[NOVA_MAX_VOICES];
} nova_machine;

/* cart.c */
int  nova_cart_open(nova_cart *c, const uint8_t *data, size_t size);
int  nova_cart_find(const nova_cart *c, uint32_t tag, const uint8_t **out, uint32_t *outlen);
void nova_cart_close(nova_cart *c);

/* mem.c */
int      nova_mem_init(nova_mem *m, const nova_cart *c);
void     nova_mem_free(nova_mem *m);
int32_t  nova_mem_load(nova_mem *m, uint32_t addr);
void     nova_mem_store(nova_mem *m, uint32_t addr, int32_t val);
void     nova_mem_bank(nova_mem *m, int bank);
void     nova_mem_dma(nova_mem *m, uint32_t dst, uint32_t src, uint32_t len);

/* vm.c */
int  nova_vm_init(nova_vm *vm, nova_machine *mc);
void nova_vm_free(nova_vm *vm);
int  nova_vm_step(nova_machine *mc);
int  nova_vm_run(nova_machine *mc, int max_steps);

/* machine.c */
int  nova_machine_load(nova_machine *mc, const uint8_t *data, size_t size);
void nova_machine_free(nova_machine *mc);
int  nova_machine_run(nova_machine *mc, int max_ticks);
void nova_machine_tick(nova_machine *mc);

/* subsystem entry points (defined per module) */
int  nova_gfx_init(nova_machine *mc);
int  nova_pal_init(nova_machine *mc);
int  nova_map_init(nova_machine *mc);
int  nova_synth_init(nova_machine *mc);
int  nova_tracker_init(nova_machine *mc);
int  nova_font_init(nova_machine *mc);
int  nova_world_init(nova_machine *mc);

void nova_gfx_free(nova_machine *mc);
void nova_pal_free(nova_machine *mc);
void nova_map_free(nova_machine *mc);
void nova_synth_free(nova_machine *mc);
void nova_tracker_free(nova_machine *mc);
void nova_font_free(nova_machine *mc);
void nova_world_free(nova_machine *mc);

void nova_gfx_sprite(nova_machine *mc, int id, int x, int y);
void nova_gfx_tile(nova_machine *mc, int mx, int my, int cols, int rows);
void nova_gfx_text(nova_machine *mc, uint32_t straddr, int x, int y);
void nova_font_render(nova_machine *mc, const uint8_t *str, int len);
void nova_pal_set(nova_machine *mc, int bank);
void nova_pal_cycle(nova_machine *mc, int lo, int hi);
void nova_pal_step(nova_machine *mc);

void nova_synth_voice(nova_machine *mc, int chan, int instr);
void nova_synth_note(nova_machine *mc, int chan, int pitch);
void nova_synth_noteoff(nova_machine *mc, int chan);
void nova_synth_tick(nova_machine *mc);

void nova_tracker_play(nova_machine *mc, int order);
void nova_tracker_tick(nova_machine *mc);

int  nova_world_spawn(nova_machine *mc, int sprite, int x, int y);
void nova_world_kill(nova_machine *mc, int handle);
void nova_world_step(nova_machine *mc);

int  nova_savestate_save(nova_machine *mc);
int  nova_savestate_load(nova_machine *mc, const uint8_t *data, uint32_t size);

/* audio_render.c */
#define NOVA_AUDIO_BLOCK 256
#define NOVA_SAMPLE_RATE 22050
void nova_audio_render(nova_machine *mc);
void nova_audio_free(nova_machine *mc);
int  nova_note_period(int pitch);

/* raster.c */
#define NOVA_BLEND_COPY  0
#define NOVA_BLEND_ADD   1
#define NOVA_BLEND_SUB   2
#define NOVA_BLEND_OR    3
void nova_raster_clear(nova_machine *mc, uint8_t color);
void nova_raster_pixel(nova_machine *mc, int x, int y, uint8_t color);
void nova_raster_hline(nova_machine *mc, int x0, int x1, int y, uint8_t color);
void nova_raster_vline(nova_machine *mc, int x, int y0, int y1, uint8_t color);
void nova_raster_line(nova_machine *mc, int x0, int y0, int x1, int y1, uint8_t color);
void nova_raster_rect(nova_machine *mc, int x, int y, int w, int h, uint8_t color);
void nova_raster_fill(nova_machine *mc, int x, int y, int w, int h, uint8_t color);
void nova_raster_circle(nova_machine *mc, int cx, int cy, int r, uint8_t color);
void nova_raster_blit_scaled(nova_machine *mc, int id, int x, int y, int sx, int sy, int blend);

/* effects.c */
void nova_fx_reset(nova_machine *mc);
void nova_fx_apply(nova_machine *mc, int chan, int cmd, int param);
void nova_fx_tick(nova_machine *mc);

/* disasm.c */
int  nova_disasm_one(const uint8_t *code, uint32_t size, uint32_t pc, char *out, int outsz);
int  nova_disasm_range(const uint8_t *code, uint32_t size, char *out, int outsz);

/* crc.c */
uint32_t nova_crc32(const uint8_t *p, size_t n);
uint32_t nova_adler32(const uint8_t *p, size_t n);
uint16_t nova_crc16(const uint8_t *p, size_t n);
uint32_t nova_fnv1a(const uint8_t *p, size_t n);

/* rng.c */
typedef struct { uint32_t s[4]; } nova_rng;
void     nova_rng_seed(nova_rng *r, uint32_t seed);
uint32_t nova_rng_next(nova_rng *r);
int      nova_rng_range(nova_rng *r, int lo, int hi);
void     nova_rng_shuffle(nova_rng *r, int *arr, int n);

/* mathlib.c (16.16 fixed point) */
#define NOVA_FP_ONE 65536
int32_t nova_fp_mul(int32_t a, int32_t b);
int32_t nova_fp_div(int32_t a, int32_t b);
int32_t nova_fp_sin(int32_t ang);
int32_t nova_fp_cos(int32_t ang);
int32_t nova_fp_sqrt(int32_t v);
int     nova_lerp(int a, int b, int t, int tmax);
int     nova_clampi(int v, int lo, int hi);

/* strtab.c */
typedef struct {
    char **items;
    int    n, cap;
} nova_strtab;
void nova_strtab_init(nova_strtab *st);
int  nova_strtab_add(nova_strtab *st, const char *s);
int  nova_strtab_find(nova_strtab *st, const char *s);
const char *nova_strtab_get(nova_strtab *st, int idx);
void nova_strtab_free(nova_strtab *st);

/* inflate.c */
int nova_rle_decode(const uint8_t *in, size_t inlen, uint8_t *out, size_t outcap);
int nova_lz_decode(const uint8_t *in, size_t inlen, uint8_t *out, size_t outcap);

/* gpu.c -- display-list coprocessor */
enum {
    GOP_END = 0, GOP_CLEAR, GOP_PIXEL, GOP_LINE, GOP_RECT, GOP_FILL,
    GOP_CIRCLE, GOP_SPRITE, GOP_SCALED, GOP_CAM, GOP_PAL
};
int nova_gpu_run(nova_machine *mc, const uint8_t *prog, uint32_t len);

/* anim.c -- sprite animation controllers */
#define NOVA_ANIM_FRAMES 32
typedef struct {
    int  frames[NOVA_ANIM_FRAMES];
    int  nframes;
    int  speed;
    int  cur;
    int  timer;
    int  loop;
    int  done;
} nova_anim;
void nova_anim_init(nova_anim *a, const int *frames, int n, int speed, int loop);
void nova_anim_step(nova_anim *a);
int  nova_anim_frame(const nova_anim *a);
void nova_anim_reset(nova_anim *a);

/* scene.c -- lightweight entity system */
#define NOVA_SCENE_MAX 128
typedef struct {
    int x, y, vx, vy;
    int sprite;
    int alive;
    int hp;
    nova_anim anim;
} nova_entity;
typedef struct {
    nova_entity *ents;
    int          n, cap;
    int          bounds_w, bounds_h;
} nova_scene;
void nova_scene_init(nova_scene *s, int w, int h);
int  nova_scene_spawn(nova_scene *s, int x, int y, int sprite);
void nova_scene_update(nova_scene *s);
void nova_scene_draw(nova_machine *mc, nova_scene *s);
void nova_scene_free(nova_scene *s);

/* path.c -- A* over a tile grid */
int nova_path_find(const uint8_t *grid, int w, int h,
                   int sx, int sy, int dx, int dy,
                   int *out_xy, int out_cap);

/* wave.c -- PCM sample chunk */
typedef struct {
    int16_t *samples;
    int      nsamples;
    int      rate;
    int      pos;
    int      playing;
} nova_wave;
int  nova_wave_load(nova_wave *w, const uint8_t *data, uint32_t len);
int  nova_wave_mix(nova_wave *w, int16_t *out, int n);
void nova_wave_free(nova_wave *w);

/* text_layout.c */
int nova_text_wrap(const char *text, int width, char *out, int outcap);
int nova_text_measure(const char *text);
void nova_text_align(char *line, int width, int mode);

/* blitter.c -- affine sprite transform */
void nova_blit_affine(nova_machine *mc, int id, int cx, int cy, int angle, int scale);
void nova_blit_rotozoom(nova_machine *mc, int id, int x, int y, int angle, int sx, int sy);

/* particles.c */
#define NOVA_MAX_PARTICLES 256
typedef struct {
    int x, y, vx, vy, life, color;
    int active;
} nova_particle;
typedef struct {
    nova_particle *p;
    int            n, cap;
    nova_rng       rng;
    int            gravity;
} nova_psys;
void nova_psys_init(nova_psys *ps, uint32_t seed);
void nova_psys_emit(nova_psys *ps, int x, int y, int count, int color);
void nova_psys_update(nova_psys *ps);
void nova_psys_draw(nova_machine *mc, nova_psys *ps);
void nova_psys_free(nova_psys *ps);

/* cmdconsole.c -- debug command line */
int nova_console_exec(nova_machine *mc, const char *line);

/* atlas.c -- sprite atlas codec */
int nova_atlas_pack(const uint8_t *pixels, int w, int h, uint8_t *out, int outcap);
int nova_atlas_unpack(const uint8_t *in, int inlen, uint8_t *out, int outcap);

/* matrix.c -- 16.16 fixed-point 2D/3D linear algebra */
typedef struct { int32_t m[9]; } nova_mat3;
typedef struct { int32_t x, y, z; } nova_vec3;
void     nova_mat3_identity(nova_mat3 *m);
nova_mat3 nova_mat3_mul(nova_mat3 a, nova_mat3 b);
nova_mat3 nova_mat3_rotz(int angle);
nova_mat3 nova_mat3_scale(int32_t sx, int32_t sy);
nova_mat3 nova_mat3_translate(int32_t tx, int32_t ty);
nova_vec3 nova_mat3_apply(nova_mat3 m, nova_vec3 v);
nova_vec3 nova_vec3_add(nova_vec3 a, nova_vec3 b);
int32_t  nova_vec3_dot(nova_vec3 a, nova_vec3 b);

/* mesh.c -- wireframe 3D */
#define NOVA_MESH_MAXV 256
#define NOVA_MESH_MAXE 512
typedef struct {
    nova_vec3 verts[NOVA_MESH_MAXV];
    int       nverts;
    int       edges[NOVA_MESH_MAXE][2];
    int       nedges;
    int       angle_x, angle_y, angle_z;
} nova_mesh;
int  nova_mesh_load(nova_mesh *me, const uint8_t *data, uint32_t len);
void nova_mesh_cube(nova_mesh *me, int32_t size);
void nova_mesh_draw(nova_machine *mc, nova_mesh *me, int cx, int cy, uint8_t color);

/* noise.c */
typedef struct { uint8_t perm[512]; } nova_noise;
void    nova_noise_init(nova_noise *n, uint32_t seed);
int32_t nova_noise_value(nova_noise *n, int32_t x, int32_t y);
int32_t nova_noise_fbm(nova_noise *n, int32_t x, int32_t y, int octaves);

/* bignum.c -- fixed 128-bit unsigned */
typedef struct { uint32_t w[4]; } nova_u128;
void nova_u128_set(nova_u128 *a, uint32_t v);
void nova_u128_add(nova_u128 *a, const nova_u128 *b);
void nova_u128_mul_u32(nova_u128 *a, uint32_t b);
int  nova_u128_cmp(const nova_u128 *a, const nova_u128 *b);
int  nova_u128_to_dec(const nova_u128 *a, char *out, int outcap);

/* config.c -- INI-style key/value config */
typedef struct {
    char **keys;
    char **vals;
    int    n, cap;
} nova_config;
void nova_config_init(nova_config *c);
int  nova_config_parse(nova_config *c, const char *text);
const char *nova_config_get(nova_config *c, const char *key);
int  nova_config_get_int(nova_config *c, const char *key, int def);
void nova_config_free(nova_config *c);

/* sound_fx.c */
void nova_sfx_echo(int16_t *buf, int n, int delay, int feedback);
void nova_sfx_lowpass(int16_t *buf, int n, int cutoff);
void nova_sfx_highpass(int16_t *buf, int n, int cutoff);
void nova_sfx_gain(int16_t *buf, int n, int gain_q8);
void nova_sfx_clip(int16_t *buf, int n, int level);

/* dither.c */
void nova_dither_ordered(uint8_t *fb, int w, int h, int levels);
int  nova_quantize(const uint32_t *pal, int npal, uint32_t rgb);
void nova_grayscale(uint8_t *fb, int w, int h);

/* save_migrate.c */
int nova_save_migrate(const uint8_t *in, int inlen, uint8_t *out, int outcap);
int nova_save_version(const uint8_t *in, int inlen);

/* tilemap_ex.c -- layered scrolling tilemap */
#define NOVA_TM_LAYERS 3
typedef struct {
    uint16_t *cells[NOVA_TM_LAYERS];
    int       w, h;
    int       scroll_x[NOVA_TM_LAYERS];
    int       scroll_y[NOVA_TM_LAYERS];
    int       nlayers;
} nova_tilemap_ex;
int  nova_tmx_init(nova_tilemap_ex *t, int w, int h, int layers);
void nova_tmx_scroll(nova_tilemap_ex *t, int layer, int dx, int dy);
int  nova_tmx_autotile(nova_tilemap_ex *t, int layer, int x, int y);
void nova_tmx_draw(nova_machine *mc, nova_tilemap_ex *t);
void nova_tmx_free(nova_tilemap_ex *t);

/* hashmap.c -- open-addressing string->int */
typedef struct { char *key; int val; int used; } nova_hm_slot;
typedef struct { nova_hm_slot *slots; int cap, n; } nova_hashmap;
int  nova_hm_init(nova_hashmap *h, int cap);
int  nova_hm_put(nova_hashmap *h, const char *key, int val);
int  nova_hm_get(nova_hashmap *h, const char *key, int *out);
int  nova_hm_del(nova_hashmap *h, const char *key);
void nova_hm_free(nova_hashmap *h);

/* ringbuf.c */
typedef struct { uint8_t *buf; int cap, head, tail, count; } nova_ring;
int  nova_ring_init(nova_ring *r, int cap);
int  nova_ring_push(nova_ring *r, uint8_t v);
int  nova_ring_pop(nova_ring *r, uint8_t *out);
int  nova_ring_count(const nova_ring *r);
void nova_ring_free(nova_ring *r);

/* bitset.c */
typedef struct { uint32_t *words; int nbits; } nova_bitset;
int  nova_bitset_init(nova_bitset *b, int nbits);
void nova_bitset_set(nova_bitset *b, int i);
void nova_bitset_clear(nova_bitset *b, int i);
int  nova_bitset_test(nova_bitset *b, int i);
int  nova_bitset_count(nova_bitset *b);
void nova_bitset_free(nova_bitset *b);

/* tween.c */
int32_t nova_ease_linear(int32_t t);
int32_t nova_ease_in_quad(int32_t t);
int32_t nova_ease_out_quad(int32_t t);
int32_t nova_ease_inout_quad(int32_t t);
int32_t nova_ease_out_bounce(int32_t t);
int32_t nova_tween(int32_t a, int32_t b, int32_t t, int kind);

/* palette_ops.c */
void nova_pal_rotate(uint32_t *pal, int n, int amount);
void nova_pal_gradient(uint32_t *pal, int n, uint32_t a, uint32_t b);
uint32_t nova_rgb_blend(uint32_t a, uint32_t b, int t);
uint32_t nova_rgb_scale(uint32_t c, int q8);

/* quadtree.c */
typedef struct nova_qnode nova_qnode;
typedef struct {
    nova_qnode *root;
    int         maxdepth;
    int         count;
} nova_quadtree;
int  nova_qt_init(nova_quadtree *q, int x, int y, int w, int h, int maxdepth);
int  nova_qt_insert(nova_quadtree *q, int x, int y, int id);
int  nova_qt_query(nova_quadtree *q, int x, int y, int w, int h, int *out, int outcap);
void nova_qt_free(nova_quadtree *q);

/* fsm.c -- entity state machine */
#define NOVA_FSM_STATES 16
typedef struct {
    int  state;
    int  timer;
    int  trans[NOVA_FSM_STATES][4];
    int  nstates;
} nova_fsm;
void nova_fsm_init(nova_fsm *f, int nstates);
void nova_fsm_set_trans(nova_fsm *f, int from, int event, int to);
int  nova_fsm_fire(nova_fsm *f, int event);
void nova_fsm_tick(nova_fsm *f);

/* coproc.c -- a second, register-based command processor (blitter VM) */
int nova_coproc_run(nova_machine *mc, const uint8_t *prog, uint32_t len);

/* spritebatch.c */
#define NOVA_BATCH_MAX 256
typedef struct { int id, x, y, z; } nova_sprite_cmd;
typedef struct { nova_sprite_cmd cmds[NOVA_BATCH_MAX]; int n; } nova_batch;
void nova_batch_init(nova_batch *b);
int  nova_batch_add(nova_batch *b, int id, int x, int y, int z);
void nova_batch_flush(nova_machine *mc, nova_batch *b);

/* image_filter.c */
void nova_filter_blur(uint8_t *fb, int w, int h);
void nova_filter_sharpen(uint8_t *fb, int w, int h);
void nova_filter_edges(uint8_t *fb, int w, int h);

/* grid.c -- generic byte grid */
typedef struct { uint8_t *cells; int w, h; } nova_grid;
int  nova_grid_init(nova_grid *g, int w, int h);
void nova_grid_set(nova_grid *g, int x, int y, uint8_t v);
uint8_t nova_grid_get(const nova_grid *g, int x, int y);
void nova_grid_fill(nova_grid *g, uint8_t v);
int  nova_grid_flood(nova_grid *g, int x, int y, uint8_t from, uint8_t to);
void nova_grid_free(nova_grid *g);

/* maze.c / dungeon.c / wfc.c -- procedural generation into a grid */
int nova_maze_gen(nova_grid *g, uint32_t seed);
int nova_dungeon_gen(nova_grid *g, uint32_t seed, int max_rooms);
int nova_wfc_gen(nova_grid *g, uint32_t seed, int ntiles);

/* lz77.c -- encoder paired with nova_lz_decode */
int nova_lz_encode(const uint8_t *in, int inlen, uint8_t *out, int outcap);

/* huffman.c */
int nova_huff_encode(const uint8_t *in, int inlen, uint8_t *out, int outcap);
int nova_huff_decode(const uint8_t *in, int inlen, uint8_t *out, int outcap);

/* base64.c */
int nova_b64_encode(const uint8_t *in, int inlen, char *out, int outcap);
int nova_b64_decode(const char *in, int inlen, uint8_t *out, int outcap);

/* camera.c */
typedef struct {
    int x, y, tx, ty;
    int shake, shake_seed;
    int bx, by, bw, bh;
} nova_camera;
void nova_camera_init(nova_camera *c, int bw, int bh);
void nova_camera_follow(nova_camera *c, int tx, int ty);
void nova_camera_shake(nova_camera *c, int amount);
void nova_camera_update(nova_camera *c);
void nova_camera_apply(nova_machine *mc, nova_camera *c);

/* timer.c */
typedef struct { int t, period, active, fired; } nova_timer;
void nova_timer_set(nova_timer *tm, int period);
int  nova_timer_update(nova_timer *tm);
void nova_timer_stop(nova_timer *tm);

/* event_queue.c */
typedef struct { int type, a, b; } nova_event;
typedef struct { nova_event *ev; int cap, head, tail, count; } nova_eventq;
int  nova_evq_init(nova_eventq *q, int cap);
int  nova_evq_push(nova_eventq *q, int type, int a, int b);
int  nova_evq_pop(nova_eventq *q, nova_event *out);
void nova_evq_free(nova_eventq *q);

/* score.c */
typedef struct { nova_u128 value; nova_u128 high; int multiplier; } nova_score;
void nova_score_init(nova_score *s);
void nova_score_add(nova_score *s, uint32_t points);
void nova_score_combo(nova_score *s, int mult);
int  nova_score_commit_high(nova_score *s);
int  nova_score_str(nova_score *s, char *out, int outcap);

/* vec2.c -- 16.16 fixed-point 2D vectors */
typedef struct { int32_t x, y; } nova_vec2;
nova_vec2 nova_v2_add(nova_vec2 a, nova_vec2 b);
nova_vec2 nova_v2_sub(nova_vec2 a, nova_vec2 b);
nova_vec2 nova_v2_scale(nova_vec2 a, int32_t s);
int32_t   nova_v2_dot(nova_vec2 a, nova_vec2 b);
int32_t   nova_v2_cross(nova_vec2 a, nova_vec2 b);
int32_t   nova_v2_len(nova_vec2 a);
nova_vec2 nova_v2_normalize(nova_vec2 a);
nova_vec2 nova_v2_rotate(nova_vec2 a, int angle);
/* vecmath.c -- extra vec2 helpers */
nova_vec2 nova_v2_lerp(nova_vec2 a, nova_vec2 b, int32_t t);
nova_vec2 nova_v2_reflect(nova_vec2 v, nova_vec2 n);
nova_vec2 nova_v2_perp(nova_vec2 a);
int32_t   nova_v2_dist(nova_vec2 a, nova_vec2 b);
nova_vec2 nova_v2_project(nova_vec2 a, nova_vec2 onto);

/* rect.c */
typedef struct { int x, y, w, h; } nova_rect;
int       nova_rect_contains(nova_rect r, int x, int y);
int       nova_rect_intersect(nova_rect a, nova_rect b, nova_rect *out);
nova_rect nova_rect_union(nova_rect a, nova_rect b);
nova_rect nova_rect_clamp(nova_rect r, nova_rect bounds);

/* color.c */
uint32_t nova_hsv_to_rgb(int h, int s, int v);
void     nova_rgb_to_hsv(uint32_t rgb, int *h, int *s, int *v);
uint32_t nova_color_lerp_hsv(uint32_t a, uint32_t b, int t);

/* sort.c */
void nova_sort_int(int *a, int n);
int  nova_bsearch_int(const int *a, int n, int key);
void nova_sort_u32(uint32_t *a, int n);

/* list.c -- doubly linked int list */
typedef struct nova_lnode { int val; struct nova_lnode *prev, *next; } nova_lnode;
typedef struct { nova_lnode *head, *tail; int count; } nova_list;
void nova_list_init(nova_list *l);
int  nova_list_push_back(nova_list *l, int v);
int  nova_list_push_front(nova_list *l, int v);
int  nova_list_pop_front(nova_list *l, int *out);
int  nova_list_remove(nova_list *l, int v);
void nova_list_free(nova_list *l);

/* hexdump.c */
int nova_hexdump(const uint8_t *data, int len, char *out, int outcap);

/* checksum2.c */
uint32_t nova_djb2(const uint8_t *p, size_t n);
uint32_t nova_sdbm(const uint8_t *p, size_t n);
uint32_t nova_murmur3(const uint8_t *p, size_t n, uint32_t seed);

/* bezier.c */
nova_vec2 nova_bezier_quad(nova_vec2 a, nova_vec2 b, nova_vec2 c, int32_t t);
nova_vec2 nova_bezier_cubic(nova_vec2 a, nova_vec2 b, nova_vec2 c, nova_vec2 d, int32_t t);
void      nova_bezier_draw(nova_machine *mc, nova_vec2 a, nova_vec2 b, nova_vec2 c, nova_vec2 d, int segs, uint8_t color);

/* polygon.c */
int  nova_poly_contains(const int *xy, int n, int px, int py);
void nova_poly_fill(nova_machine *mc, const int *xy, int n, uint8_t color);
int  nova_poly_area2(const int *xy, int n);

/* astar_grid.c -- weighted A* over a terrain-cost grid (0 = blocked) */
int nova_astar_cost(const uint8_t *cost, int w, int h, int sx, int sy, int dx, int dy, int *out_xy, int outcap);

/* state_stack.c -- game state stack */
#define NOVA_STATE_MAX 16
typedef struct { int states[NOVA_STATE_MAX]; int top; } nova_sstack;
void nova_sstack_init(nova_sstack *s);
int  nova_sstack_push(nova_sstack *s, int state);
int  nova_sstack_pop(nova_sstack *s);
int  nova_sstack_top(const nova_sstack *s);

/* anim_track.c -- keyframe interpolation */
#define NOVA_TRACK_KEYS 32
typedef struct { int time; int value; } nova_keyframe;
typedef struct { nova_keyframe keys[NOVA_TRACK_KEYS]; int nkeys; } nova_track;
void nova_track_init(nova_track *t);
int  nova_track_add(nova_track *t, int time, int value);
int  nova_track_sample(const nova_track *t, int time);

/* spring.c -- damped spring */
typedef struct { int32_t pos, vel, target, k, damp; } nova_spring;
void nova_spring_init(nova_spring *s, int32_t pos, int32_t k, int32_t damp);
void nova_spring_update(nova_spring *s);

/* validate.c -- cartridge linter */
int nova_validate_cart(const uint8_t *data, size_t len, char *report, int cap);

/* obj_loader.c -- text mesh format into nova_mesh */
int nova_obj_load(nova_mesh *me, const char *text, int len);

/* audio_seq.c -- step arpeggiator */
#define NOVA_ARP_STEPS 16
typedef struct { int root; int steps[NOVA_ARP_STEPS]; int n; int pos; int gate; } nova_arp;
void nova_arp_init(nova_arp *a, int root);
int  nova_arp_set(nova_arp *a, const int *offs, int n);
int  nova_arp_next(nova_arp *a);

/* bmp.c -- framebuffer to grayscale PGM bytes */
int nova_fb_to_pgm(const uint8_t *fb, int w, int h, char *out, int outcap);

/* text_render.c -- draw font glyphs into the framebuffer */
void nova_font_blit(nova_machine *mc, int glyph, int x, int y, uint8_t color);
int  nova_text_draw(nova_machine *mc, const char *str, int x, int y, uint8_t color);
int  nova_text_draw_wrapped(nova_machine *mc, const char *str, int x, int y, int width, uint8_t color);

/* wavetable.c */
int nova_wavetable_gen(int kind, int16_t *out, int n);
int nova_wavetable_mix(const int16_t *a, const int16_t *b, int16_t *out, int n, int balance);

/* blend.c -- framebuffer compositing */
void nova_fb_blend(uint8_t *dst, const uint8_t *src, int n, int mode);
void nova_fb_mask(uint8_t *dst, const uint8_t *src, const uint8_t *mask, int n);
void nova_fb_fade(uint8_t *fb, int n, int amount);

/* minimap.c */
void nova_minimap_draw(nova_machine *mc, const uint16_t *tiles, int w, int h, int ox, int oy, int scale);

/* ui.c -- immediate-mode widgets drawn via the rasterizer */
void nova_ui_panel(nova_machine *mc, int x, int y, int w, int h, uint8_t fill, uint8_t border);
int  nova_ui_button(nova_machine *mc, int x, int y, int w, int h, const char *label, int hot);
void nova_ui_label(nova_machine *mc, int x, int y, const char *text, uint8_t color);
void nova_ui_bar(nova_machine *mc, int x, int y, int w, int h, int value, int max, uint8_t color);
void nova_ui_progress(nova_machine *mc, int x, int y, int w, int value_q8);

/* softsynth.c -- wavetable voices with an ADSR envelope */
typedef struct {
    int  active;
    int  wave;
    uint32_t phase, step;
    int  vol;
    int  env_stage, env_level;
    int  attack, decay, sustain, release;
} nova_softvoice;
typedef struct {
    nova_softvoice voices[NOVA_MAX_VOICES];
    int16_t tables[6][256];
    int     tables_ready;
} nova_softsynth;
void nova_softsynth_init(nova_softsynth *s);
void nova_softsynth_noteon(nova_softsynth *s, int ch, int pitch, int wave);
void nova_softsynth_noteoff(nova_softsynth *s, int ch);
int  nova_softsynth_render(nova_softsynth *s, int16_t *out, int n);

/* inputmap.c -- button state with edge detection */
typedef struct { uint32_t cur, prev; } nova_input;
void nova_input_init(nova_input *in);
void nova_input_set(nova_input *in, uint32_t mask);
void nova_input_update(nova_input *in);
int  nova_input_pressed(const nova_input *in, int bit);
int  nova_input_held(const nova_input *in, int bit);
int  nova_input_released(const nova_input *in, int bit);

/* dialogue.c -- typewriter text */
typedef struct {
    char text[512];
    int  len, pos, speed, timer, done;
} nova_dialogue;
void nova_dialogue_set(nova_dialogue *d, const char *text, int speed);
void nova_dialogue_advance(nova_dialogue *d);
int  nova_dialogue_visible(nova_dialogue *d, char *out, int outcap);
void nova_dialogue_skip(nova_dialogue *d);

/* inventory.c */
#define NOVA_INV_SLOTS 32
typedef struct { int item, count; } nova_inv_slot;
typedef struct { nova_inv_slot slots[NOVA_INV_SLOTS]; int n; int stack_max; } nova_inventory;
void nova_inv_init(nova_inventory *inv, int stack_max);
int  nova_inv_add(nova_inventory *inv, int item, int count);
int  nova_inv_remove(nova_inventory *inv, int item, int count);
int  nova_inv_count(const nova_inventory *inv, int item);

/* tilecollide.c -- resolve an AABB against solid tiles in a grid */
int nova_tile_resolve(const uint8_t *grid, int w, int h, int tile,
                      int *x, int *y, int bw, int bh, int vx, int vy);

/* automata.c -- cellular automata over a grid (cave generation) */
int nova_ca_step(nova_grid *g, int birth, int survive);
int nova_ca_cave(nova_grid *g, uint32_t seed, int iters, int fill_pct);

/* heap.c -- binary min-heap priority queue (key, value) */
typedef struct { int *key; int *val; int n, cap; } nova_heap;
int  nova_heap_init(nova_heap *h, int cap);
int  nova_heap_push(nova_heap *h, int key, int val);
int  nova_heap_pop(nova_heap *h, int *key, int *val);
int  nova_heap_peek(const nova_heap *h, int *key, int *val);
void nova_heap_free(nova_heap *h);

/* steering.c -- vec2 steering behaviours */
nova_vec2 nova_steer_seek(nova_vec2 pos, nova_vec2 target, int32_t maxspeed);
nova_vec2 nova_steer_flee(nova_vec2 pos, nova_vec2 target, int32_t maxspeed);
nova_vec2 nova_steer_arrive(nova_vec2 pos, nova_vec2 target, int32_t maxspeed, int32_t slow_radius);
nova_vec2 nova_steer_wander(nova_vec2 vel, nova_rng *r, int32_t jitter);

/* dice.c -- random helpers */
int nova_roll(nova_rng *r, int sides, int count);
int nova_weighted_choice(nova_rng *r, const int *weights, int n);
typedef struct { int *items; int n, cap, pos; nova_rng rng; } nova_bag;
int  nova_bag_init(nova_bag *b, int cap, uint32_t seed);
int  nova_bag_add(nova_bag *b, int item);
int  nova_bag_draw(nova_bag *b);
void nova_bag_free(nova_bag *b);

/* deque.c -- double-ended int queue */
typedef struct { int *buf; int cap, head, tail, count; } nova_deque;
int  nova_deque_init(nova_deque *d, int cap);
int  nova_deque_push_back(nova_deque *d, int v);
int  nova_deque_push_front(nova_deque *d, int v);
int  nova_deque_pop_back(nova_deque *d, int *out);
int  nova_deque_pop_front(nova_deque *d, int *out);
void nova_deque_free(nova_deque *d);

/* json.c -- minimal JSON DOM */
enum { NJSON_NULL, NJSON_BOOL, NJSON_NUM, NJSON_STR, NJSON_ARR, NJSON_OBJ };
typedef struct {
    int   type;
    long  num;
    int   first_child;
    int   next_sibling;
    char  key[48];
    char  str[48];
} nova_json_node;
typedef struct {
    nova_json_node *nodes;
    int             n, cap;
    const char     *src;
    int             pos, len, err;
} nova_json;
int nova_json_parse(nova_json *j, const char *text, int len);
int nova_json_get(nova_json *j, int obj, const char *key);
void nova_json_free(nova_json *j);

/* collision2.c */
typedef struct { int x, y, w, h; } nova_aabb;
int nova_aabb_overlap(nova_aabb a, nova_aabb b);
int nova_aabb_sweep(nova_aabb a, int vx, int vy, nova_aabb b, int *tx, int *ty);
typedef struct {
    int   cell;
    int  *buckets;
    int  *next;
    int   nbuckets;
    int   nitems, capitems;
    nova_aabb *boxes;
} nova_spatial;
int  nova_spatial_init(nova_spatial *s, int cell, int nbuckets, int capitems);
int  nova_spatial_insert(nova_spatial *s, nova_aabb box);
int  nova_spatial_query(nova_spatial *s, nova_aabb box, int *out, int outcap);
void nova_spatial_free(nova_spatial *s);

#endif
