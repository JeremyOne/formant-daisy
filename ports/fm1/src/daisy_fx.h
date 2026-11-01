/* Formant Daisy, fixed-point engine for small hardware (M-VAVE FM-1 and similar).
 *
 * A port of the plugin's FormantSinger (Source/engine/FormantSinger.cpp) to C99
 * with integer arithmetic only: no float, no malloc, no libc beyond memset.
 * One voice (Daisy sings one line); 44.1 kHz; a control tick every 16 samples.
 *
 * Lyrics arrive already turned into phonemes (the device has no room for the
 * pronouncing dictionary): see "Lyric data" below and tools/fm1_encode.cpp,
 * which uses the plugin's own text-to-phoneme code to make them.
 *
 *   dfx_init(&s);                       once
 *   dfx_set_lyrics(&s, bytes, len);     a lyric in the format below (kept by pointer, not copied)
 *   dfx_note_on(&s, note, vel);         sings the next syllable at that pitch
 *   dfx_note_off(&s, note);             releases it (the coda is sung)
 *   dfx_render(&s, out, n);             adds n samples (Q15 scale) to out
 */
#ifndef DAISY_FX_H
#define DAISY_FX_H
#include <stdint.h>

#define DFX_FS 44100
#define DFX_CTL 16            /* samples per control tick (as the plugin) */
#define DFX_MAX_SYL 512       /* syllables in a lyric */
#define DFX_MAX_LYRIC 4096    /* bytes of lyric data */
#define DFX_QUEUE 24          /* queued segments */
#define DFX_MAX_PH 6          /* phonemes in an onset or coda */

/* ---- Lyric data ---------------------------------------------------------
 * A sequence of syllable records, every byte 0..127 (so it can travel in a
 * MIDI SysEx message unchanged):
 *   flags                bit0 rest (a punctuation mark: uses one note, silent)
 *                        bit1 a line break follows
 *   n_onset, onset[n]    phoneme codes 1..40
 *   nucleus              the vowel
 *   n_coda, coda[n]
 * A rest record has n_onset = 0, nucleus = 0, n_coda = 0.
 * Phoneme codes follow tools/make_cmudict.py's order: 1 iy .. 40 y, 0 silence. */
enum { DFX_REST = 1, DFX_LINEBREAK = 2 };

typedef struct {
    int16_t source;     /* 0 buzz .. 256 glottal pulse */
    int16_t breath;     /* 0 .. 256 */
    int16_t shift;      /* formant scale, 256 = 1.0 (180 .. 384) */
    int16_t speed;      /* consonant length, 256 = 1.0 */
    int16_t glide_ms;   /* pitch glide between overlapping notes */
    int16_t vib_cents;  /* vibrato depth */
    int16_t vib_rate16; /* vibrato rate, Hz * 16 */
    int16_t gain;       /* output, 256 = 1.0 */
    uint8_t loop;       /* start the lyric over when it runs out */
} dfx_params_t;

typedef struct {        /* a frame of synthesis targets */
    int32_t F[5], B[5];     /* Hz * 16 */
    int32_t fnz;            /* nasal zero, Hz * 16 */
    int32_t fr1f, fr1b, fr2f, fr2b; /* frication resonators, Hz * 16 */
    int32_t fr1a, fr2a, byp;        /* Q15 */
    int32_t av, af, ah;             /* Q15 */
} dfx_frame_t;

typedef struct {
    dfx_frame_t tgt;
    int32_t dur;        /* samples, -1 = hold until the next segment arrives */
    uint8_t ftau, atau; /* smoothing time constants, ms */
    uint8_t ph, nucleus;
} dfx_seg_t;

typedef struct { int32_t a, b, c; int32_t y1, y2; } dfx_res_t;      /* y = a x + b y1 + c y2, Q30 */
typedef struct { int32_t a, b, c; int32_t x1, x2; } dfx_antires_t;  /* y = a x + b x1 + c x2, Q30 */

typedef struct {
    dfx_params_t p;

    /* lyrics */
    const uint8_t *lyric;           /* the caller's data (it must stay put while it is in use) */
    uint16_t syl_off[DFX_MAX_SYL];  /* byte offset of each syllable record */
    uint16_t nsyl, next_syl;
    int16_t cur_syl;                /* syllable being held, -1 none */

    /* segments */
    dfx_seg_t seg;                  /* the one playing */
    int32_t seg_elapsed;
    dfx_seg_t queue[DFX_QUEUE];
    uint8_t q_head, q_count;

    /* smoothed state */
    dfx_frame_t cur;
    int32_t vel_gain, vel_target;   /* Q15 */
    int32_t ctl_left;

    /* source */
    uint32_t phase, inc, inc_target;
    uint32_t vib_phase;
    int32_t tilt, tilt_coef, fric_lp;
    uint32_t rng;
    uint32_t nucleus_age;           /* control ticks since the vowel started (vibrato fade in) */

    /* filters */
    dfx_res_t casc[5], npole, fr1, fr2;
    dfx_antires_t nzero;

    /* keys held, newest last */
    uint8_t held[8], nheld;

    /* display */
    uint8_t viz_ph;                 /* phoneme being made */
    int16_t viz_syl;                /* syllable index sung last */
} dfx_singer_t;

void dfx_init(dfx_singer_t *s);
void dfx_default_params(dfx_params_t *p);
/* returns the number of syllables (0 if the data is malformed; the old lyric is kept) */
int dfx_set_lyrics(dfx_singer_t *s, const uint8_t *data, uint32_t len);
/* the number of syllables in lyric data, 0 if it is malformed (nothing is changed) */
int dfx_lyrics_valid(const uint8_t *data, uint32_t len);
void dfx_restart(dfx_singer_t *s);
void dfx_note_on(dfx_singer_t *s, uint8_t note, uint8_t vel);
void dfx_note_off(dfx_singer_t *s, uint8_t note);
void dfx_all_off(dfx_singer_t *s);
void dfx_render(dfx_singer_t *s, int32_t *out, uint32_t n);
/* a host with its own pitch handling (glide, bend, tuning): the phase increment to sing at, Q32 per sample.
 * Call before each render; set p.glide_ms to 0 so the engine doesn't glide on top. */
void dfx_set_inc(dfx_singer_t *s, uint32_t inc);
/* nothing sounding and nothing queued (a host can end the voice) */
int dfx_silent(const dfx_singer_t *s);

/* the formants being made, Hz (for a vowel chart on the screen) */
static inline int32_t dfx_f1(const dfx_singer_t *s) { return s->cur.F[0] >> 4; }
static inline int32_t dfx_f2(const dfx_singer_t *s) { return s->cur.F[1] >> 4; }

#endif
