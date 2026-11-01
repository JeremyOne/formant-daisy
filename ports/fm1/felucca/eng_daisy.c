/* SPDX-License-Identifier: GPL-3.0-only
 * DAISY: Formant Daisy's singing robot as a Felucca engine (experimental).
 *
 * Each key sings the next syllable of a lyric at that key's pitch; holding it
 * holds the vowel, letting go sings the word's ending. The lyric comes from
 * the plugin's text-to-phoneme code on a computer (ports/fm1/tools/fm1_encode):
 * two are built in (LYRC: DAISY, ROBOT) and one can be sent over USB MIDI as
 * SysEx (LYRC: USER), in pieces, as Felucca's SysEx frames hold 640 bytes:
 *   F0 7D 44 46 01 F7              begin
 *   F0 7D 44 46 02 <bytes> F7      more of the lyric (up to 512 bytes a message)
 *   F0 7D 44 46 03 F7              end: checked, then it becomes the USER lyric
 *
 * Mono: one singer per part (its state in the part's engine memory). Its own
 * envelope (engine_t.ownenv): the voice ends when the singer has gone quiet.
 * The pitch is Felucca's (glide, bend, tuning, the LFO): m->inc.
 * The sequencer starting restarts the lyric (song.playing, core.h).
 *
 * The engine itself (daisy_fx.c) is the plugin's FormantSinger ported to
 * integer C; it is included here, as Felucca builds its engines as one unit. */
#include "daisy_fx.c"
#include "daisy_lyrics.h"

#define DAISY_USER_MAX DFX_MAX_LYRIC
static uint8_t daisy_user[DAISY_USER_MAX] __attribute__((section(".pool")));
static uint8_t daisy_stage[DAISY_USER_MAX] __attribute__((section(".pool")));   /* an upload in progress */
static uint16_t daisy_user_len, daisy_stage_len;
static uint8_t daisy_user_gen;           /* bumped by each upload: the parts reload */

typedef struct {
    dfx_singer_t sg;
    uint8_t init, lyric, user_gen, playing, gate;
} daisy_part_t;
ENG_MEM_FITS("DAISY", sizeof(daisy_part_t));

static daisy_part_t *daisy_of(const track_t *t) { return (daisy_part_t *)ENG_MEM(t - trk); }

static const char *const N_DAISY_LYRIC[] = {"DAISY", "ROBOT", "USER"};

/* the lyric LYRC selects (USER falls back to DAISY until one is sent) */
static void daisy_load(daisy_part_t *d, uint32_t sel)
{
    if (sel == 2u && daisy_user_len && dfx_set_lyrics(&d->sg, daisy_user, daisy_user_len))
        ;
    else if (sel == 1u)
        dfx_set_lyrics(&d->sg, DFX_LYRIC_ROBOT, sizeof DFX_LYRIC_ROBOT);
    else
        dfx_set_lyrics(&d->sg, DFX_LYRIC_DAISY, sizeof DFX_LYRIC_DAISY);
    d->lyric = (uint8_t)sel;
    d->user_gen = daisy_user_gen;
}

/* EDIT: VOICE BREATH FORMANT CONS | VIBRATO VIB RATE LYRIC LOOP, 0..127 -> the engine's units */
static void daisy_params(daisy_part_t *d, const track_t *t)
{
    dfx_params_t *p = &d->sg.p;
    p->source = (int16_t)(t->p[P_E0] * 2);
    p->breath = (int16_t)(t->p[P_E1] * 2);
    p->shift = (int16_t)(t->p[P_E2] < 64 ? 180 + t->p[P_E2] * 76 / 64 : 256 + (t->p[P_E2] - 64) * 128 / 63);   /* 0.7 .. 1.5 */
    p->speed = (int16_t)(128 + t->p[P_E3] * 2);                                                                 /* 0.5 .. 1.5 */
    p->vib_cents = (int16_t)(t->p[P_E4] * 100 / 127);
    p->vib_rate16 = (int16_t)(32 + t->p[P_E5] * 112 / 127);                                                    /* 2 .. 9 Hz */
    p->loop = (uint8_t)(t->p[P_E7] != 0);
    p->glide_ms = 0;                                                                                            /* Felucca glides */
    p->gain = 256;
}

static daisy_part_t *daisy_ready(track_t *t)
{
    daisy_part_t *d = daisy_of(t);
    const uint32_t sel = (uint32_t)clamp(t->p[P_E6], 0, 2);
    if (!d->init) {                      /* the region is zeroed when DAISY claims it */
        dfx_init(&d->sg);
        d->init = 1;
        daisy_load(d, sel);
    }
    if (sel != d->lyric || (sel == 2u && d->user_gen != daisy_user_gen))
        daisy_load(d, sel);
    daisy_params(d, t);
    return d;
}

/* once per block, voices or not: the sequencer starting takes the lyric back to its first syllable */
static void daisy_block(track_t *t)
{
    daisy_part_t *d = daisy_ready(t);
    const uint8_t pl = (uint8_t)(song.playing != 0);
    if (pl && !d->playing)
        dfx_restart(&d->sg);
    d->playing = pl;
}

static void daisy_note_on(track_t *t, voice_t *v)
{
    daisy_part_t *d = daisy_ready(t);
    dfx_note_on(&d->sg, v->note, v->vel);
    d->gate = 1;
}

static void daisy_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    daisy_part_t *d = daisy_ready(t);
    int32_t buf[CTL];
    uint32_t i;
    if (d->gate && !v->gate) {           /* the key went up: sing the coda */
        dfx_note_off(&d->sg, v->note);
        d->gate = 0;
    }
    dfx_set_inc(&d->sg, m->inc);
    for (i = 0; i < n; i++)
        buf[i] = 0;
    dfx_render(&d->sg, buf, n);
    for (i = 0; i < n; i++)
        out[i] += voice_amp(clamp(buf[i], -32767, 32767), m, i) * 3;   /* x3: level with VOICE (which doubles) */
}

static int daisy_done(track_t *t, voice_t *v)
{
    daisy_part_t *d = daisy_of(t);
    if (d->gate && !v->gate) {
        dfx_note_off(&d->sg, v->note);
        d->gate = 0;
    }
    return !v->gate && dfx_silent(&d->sg);
}

static const preset_t DAISY_PRESETS[] = {
    /* VOICE BRTH FMNT CONS | VIB VRAT LYRC LOOP */
    {"DAISY BELL", {64, 19, 64, 64, 32, 56, 0, 1}, {0, 0, 127, 0}, 0, 1, FX(0, 10, 15, 30), 0},
    {"1961 MAINFRAME", {0, 0, 64, 64, 0, 56, 0, 1}, {0, 0, 127, 0}, 0, 1, FX(20, 0, 10, 10), 0},
    {"ROBOT HOUSE", {40, 10, 70, 56, 20, 70, 1, 1}, {0, 0, 127, 0}, 0, 1, FX(0, 30, 35, 40), 0},
    {"TINY ROBOT", {80, 25, 115, 50, 40, 90, 0, 1}, {0, 0, 127, 0}, 0, 1, FX(0, 40, 30, 50), 0},
    {"BIG ROBOT", {30, 15, 10, 80, 15, 40, 0, 1}, {0, 0, 127, 0}, 0, 1, FX(10, 0, 20, 60), 0},
};

static const engine_t ENG_DAISY = {
    .name = "DAISY",
    .page_title = {"SING", "WORD"},
    .edit = {
        {"VOICE", F_PCT, 0, 127, 64, 0, 0},
        {"BRTH", F_PCT, 0, 127, 19, 0, 0},
        {"FMNT", F_INT, 0, 127, 64, 0, 0},
        {"CONS", F_INT, 0, 127, 64, 0, 0},
        {"VIB", F_PCT, 0, 127, 32, 0, 0},
        {"VRAT", F_INT, 0, 127, 56, 0, 0},
        {"LYRC", F_ENUM, 0, 2, 0, N_DAISY_LYRIC, 0},
        {"LOOP", F_ONOFF, 0, 1, 1, 0, 0},
    },
    .presets = DAISY_PRESETS,
    .npresets = NELEM(DAISY_PRESETS),
    .knob = {P_E0, P_E2, P_E3, P_E6},
    .poly = 1,                           /* one singer */
    .note_on = daisy_note_on,
    .render = daisy_render,
    .ownenv = 1,
    .done = daisy_done,
    .block = daisy_block,
    .mem = sizeof(daisy_part_t),
};

/* engine 14 when ADPCM isn't built: reserved (never offered), so DAISY keeps number 15 either way */
#if !FELUCCA_ADPCM
static void daisy_rsv_note_on(track_t *t, voice_t *v) { (void)t; (void)v; }
static void daisy_rsv_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    (void)t; (void)v; (void)out; (void)n; (void)m;
}
static const engine_t ENG_RESERVED14 = {
    .name = "-",
    .page_title = {"-", "-"},
    .edit = {{"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0},
             {"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0}, {"-", F_INT, 0, 0, 0, 0, 0}},
    .note_on = daisy_rsv_note_on,
    .render = daisy_rsv_render,
    .knob = {P_E0, P_E1, P_E2, P_E3},
};
#endif

/* SysEx after the F0: 7D 44 46 cmd .. (editor.c ed_service hands these frames here). Returns 1 if it was one. */
static int daisy_sysex(const uint8_t *f, uint32_t n)
{
    uint32_t i;
    if (n < 4u || f[0] != 0x7D || f[1] != 0x44 || f[2] != 0x46)
        return 0;
    if (f[3] == 0x01)                    /* begin */
        daisy_stage_len = 0;
    else if (f[3] == 0x02) {             /* more */
        if (daisy_stage_len + (n - 4u) <= DAISY_USER_MAX) {
            for (i = 4; i < n; i++)
                daisy_stage[daisy_stage_len++] = f[i];
        } else
            daisy_stage_len = DAISY_USER_MAX + 1u;   /* too long: the end refuses it */
    } else if (f[3] == 0x03 && daisy_stage_len <= DAISY_USER_MAX && dfx_lyrics_valid(daisy_stage, daisy_stage_len)) {
        for (i = 0; i < daisy_stage_len; i++)
            daisy_user[i] = daisy_stage[i];
        daisy_user_len = daisy_stage_len;
        daisy_user_gen++;
        for (i = 0; i < NPART; i++)      /* the parts singing: switch them to the new lyric */
            if (trk[i].eng_req == ENGI_DAISY)
                trk[i].p[P_E6] = 2;
    }
    return 1;
}
