/* Formant Daisy, fixed-point engine. See daisy_fx.h.
 *
 * Signals inside are Q12 (4096 = 1.0) in int32, which leaves a lot of headroom
 * for the resonant peaks of the cascade. Filter coefficients are DFX_Q30 (Q20 for
 * the nasal antiresonator, whose gain can reach a few hundred). Frequencies and
 * bandwidths are Hz * 16. Amplitudes are Q15. */
#include "daisy_fx.h"
#include "daisy_fx_tables.h"
#include <string.h>

#if DFX_FS != DFX_TABLE_FS
#error "daisy_fx_tables.h was made for another sample rate"
#endif

#define DFX_Q30 (1 << 30)
#define DFX_OQ_Q16 39322 /* open quotient 0.6 */

static int32_t dfx_clampi(int32_t x, int32_t lo, int32_t hi) { return x < lo ? lo : (x > hi ? hi : x); }
static int32_t dfx_ms_to_samples(int32_t ms) { return ms * (DFX_FS / 100) / 10; }

/* ------------------------------------------------------------ filters --- */
/* dfx_pole radius r = exp(-pi B / FS) and 2 cos(2 pi F / FS), both DFX_Q30, from the tables */
static void dfx_pole(int32_t f16, int32_t b16, int32_t *b, int32_t *c)
{
    int32_t pos, idx, fr, cosv, r;
    f16 = dfx_clampi(f16, 16 * 20, 16 * (DFX_FS * 45 / 100));
    pos = (int32_t)(((int64_t)f16 * 32768) / DFX_FS);   /* (F / (FS / 2)) * 1024 * 256 */
    idx = pos >> 8;
    fr = pos & 255;
    if (idx >= 1024) { idx = 1023; fr = 256; }
    cosv = DFX_COS_Q30[idx] + (int32_t)(((int64_t)(DFX_COS_Q30[idx + 1] - DFX_COS_Q30[idx]) * fr) >> 8);
    b16 = dfx_clampi(b16, 16, 16 * 4000);
    idx = b16 >> 7;                                       /* / 128: steps of 8 Hz */
    fr = b16 & 127;
    if (idx >= 511) { idx = 510; fr = 128; }
    r = DFX_BWR_Q30[idx] + (int32_t)(((int64_t)(DFX_BWR_Q30[idx + 1] - DFX_BWR_Q30[idx]) * fr) >> 7);
    *b = (int32_t)(((int64_t)r * cosv) >> 29);            /* 2 r cos */
    *c = -(int32_t)(((int64_t)r * r) >> 30);              /* -r^2 */
}

static void dfx_res_set(dfx_res_t *f, int32_t f16, int32_t b16)
{
    dfx_pole(f16, b16, &f->b, &f->c);
    f->a = DFX_Q30 - f->b - f->c;                             /* unity gain at DC */
}

static int32_t dfx_res_run(dfx_res_t *f, int32_t x)
{
    int32_t y = (int32_t)(((int64_t)f->a * x + (int64_t)f->b * f->y1 + (int64_t)f->c * f->y2 + (DFX_Q30 >> 1)) >> 30);
    f->y2 = f->y1;
    f->y1 = y;
    return y;
}

/* Klatt's antiresonator: the inverse of a resonator. Q20 (its gain can be large) */
static void dfx_antires_set(dfx_antires_t *f, int32_t f16, int32_t b16)
{
    int32_t b, c, a;
    dfx_pole(f16, b16, &b, &c);
    a = DFX_Q30 - b - c;
    if (a < 600000) a = 600000;                          /* keeps 1 / A within Q20 int32 */
    f->a = (int32_t)(((int64_t)1 << 50) / a);
    f->b = (int32_t)(-((int64_t)b << 20) / a);
    f->c = (int32_t)(-((int64_t)c << 20) / a);
}

static int32_t dfx_antires_run(dfx_antires_t *f, int32_t x)
{
    int32_t y = (int32_t)(((int64_t)f->a * x + (int64_t)f->b * f->x1 + (int64_t)f->c * f->x2 + (1 << 19)) >> 20);
    f->x2 = f->x1;
    f->x1 = x;
    return y;
}

/* --------------------------------------------------------- phonemes --- */
static void dfx_frame_for(dfx_frame_t *f, uint8_t ph)
{
    const dfx_phone_t *d = &DFX_PHONES[ph < DFX_NPHONE ? ph : 0];
    const dfx_fric_t *fs = &DFX_FRIC[d->fric];
    f->F[0] = d->F[0] << 4; f->F[1] = d->F[1] << 4; f->F[2] = d->F[2] << 4;
    f->F[3] = 3300 << 4;    f->F[4] = 3750 << 4;
    f->B[0] = d->B[0] << 4; f->B[1] = d->B[1] << 4; f->B[2] = d->B[2] << 4;
    f->B[3] = 250 << 4;     f->B[4] = 200 << 4;
    f->av = d->av; f->af = d->af; f->ah = d->ah;
    f->fr1f = fs->f1 << 4; f->fr1b = fs->b1 << 4; f->fr1a = fs->a1;
    f->fr2f = fs->f2 << 4; f->fr2b = fs->b2 << 4; f->fr2a = fs->a2;
    f->byp = fs->byp;
    f->fnz = (d->fnz ? d->fnz : 270) << 4;               /* 270 Hz cancels the nasal dfx_pole */
}

static int dfx_is_fric(uint8_t ph)
{
    uint8_t k = DFX_PHONES[ph < DFX_NPHONE ? ph : 0].kind;
    return k == DFX_K_FRICVOICELESS || k == DFX_K_FRICVOICED || k == DFX_K_AFFRICATEVOICELESS ||
           k == DFX_K_AFFRICATEVOICED || k == DFX_K_ASPIRATE;
}

/* ---------------------------------------------------------- segments --- */
static dfx_seg_t *dfx_q_at(dfx_singer_t *s, int i) { return &s->queue[(s->q_head + i) % DFX_QUEUE]; }

static void dfx_push_seg(dfx_singer_t *s, const dfx_seg_t *g)
{
    if (s->q_count >= DFX_QUEUE) return;
    *dfx_q_at(s, s->q_count) = *g;
    s->q_count++;
}

static void dfx_mk(dfx_seg_t *g, const dfx_frame_t *f, int32_t dur_ms_x256, uint8_t ftau, uint8_t atau, uint8_t ph)
{
    memset(g, 0, sizeof *g);
    g->tgt = *f;
    g->dur = dfx_ms_to_samples(dur_ms_x256 >> 8);
    if (g->dur < 1) g->dur = 1;
    g->ftau = ftau;
    g->atau = atau;
    g->ph = ph;
}

static void dfx_push_silence(dfx_singer_t *s)
{
    dfx_seg_t g;
    memset(&g, 0, sizeof g);
    g.tgt = s->q_count ? dfx_q_at(s, s->q_count - 1)->tgt : s->seg.tgt;   /* keep the formants: no sweep */
    g.tgt.av = g.tgt.ah = g.tgt.af = 0;
    g.dur = -1;
    g.ftau = 60;
    g.atau = 20;
    dfx_push_seg(s, &g);
}

/* one consonant as the plugin makes it (FormantSinger::pushConsonant); times in ms * 256 */
static void dfx_push_consonant(dfx_singer_t *s, uint8_t ph, uint8_t nucleus, int onset, int last_before_vowel)
{
    const dfx_phone_t *d = &DFX_PHONES[ph];
    const int32_t sp = s->p.speed;                        /* 256 = 1.0 */
    const int32_t dur = (int32_t)d->dur * sp;             /* ms * 256 */
    dfx_frame_t base, vowel, f;
    dfx_seg_t g;
    dfx_frame_for(&base, ph);
    dfx_frame_for(&vowel, nucleus ? nucleus : 11 /* ax */);

    switch (d->kind) {
    case DFX_K_STOPVOICELESS:
    case DFX_K_STOPVOICED: {
        const int voiced = d->kind == DFX_K_STOPVOICED;
        f = base;
        f.af = f.ah = 0;
        f.av = voiced ? 2294 : 0;                         /* 0.07: the voice bar */
        if (voiced) f.F[0] = 180 << 4;
        dfx_mk(&g, &f, dur, 12, 5, ph);
        dfx_push_seg(s, &g);
        f = base;
        f.av = voiced ? 8192 : 0;
        f.ah = 0;
        f.af = onset ? d->af : d->af * 2 / 5;
        dfx_mk(&g, &f, (voiced ? 8 : 12) << 8, 4, 2, ph);
        dfx_push_seg(s, &g);
        if (!voiced) {
            f = vowel;                                    /* aspiration takes the vowel's shape */
            f.av = f.af = 0;
            f.ah = onset ? d->ah : d->ah * 3 / 10;
            dfx_mk(&g, &f, (onset ? (last_before_vowel ? 45 : 12) : 15) * sp, 25, 4, ph);
            dfx_push_seg(s, &g);
        }
        break;
    }
    case DFX_K_AFFRICATEVOICELESS:
    case DFX_K_AFFRICATEVOICED:
        f = base;
        f.af = f.ah = 0;
        f.av = d->kind == DFX_K_AFFRICATEVOICED ? 2294 : 0;
        dfx_mk(&g, &f, dur, 12, 4, ph);
        dfx_push_seg(s, &g);
        dfx_mk(&g, &base, 60 * sp, 10, 3, ph);
        dfx_push_seg(s, &g);
        break;
    case DFX_K_FRICVOICELESS:
    case DFX_K_FRICVOICED:
        dfx_mk(&g, &base, onset ? dur : dur * 9 / 10, 15, 10, ph);
        dfx_push_seg(s, &g);
        break;
    case DFX_K_ASPIRATE:
        f = vowel;
        f.av = f.af = 0;
        f.ah = d->ah;
        dfx_mk(&g, &f, dur, 4, 10, ph);
        dfx_push_seg(s, &g);
        break;
    case DFX_K_NASAL:
        dfx_mk(&g, &base, onset ? dur : dur * 13 / 10, 15, 10, ph);
        dfx_push_seg(s, &g);
        break;
    default:                                              /* liquids, glides */
        dfx_mk(&g, &base, dur, 25, 15, ph);
        dfx_push_seg(s, &g);
        break;
    }
}

/* ------------------------------------------------------------ lyrics --- */
/* a syllable record: flags, n_onset, onset[], nucleus, n_coda, coda[] */
typedef struct {
    uint8_t flags, n_onset, nucleus, n_coda;
    const uint8_t *onset, *coda;
} dfx_syl_t;

static void dfx_syl_at(const dfx_singer_t *s, int i, dfx_syl_t *y)
{
    const uint8_t *p = s->lyric + s->syl_off[i];
    y->flags = p[0];
    y->n_onset = p[1];
    y->onset = p + 2;
    y->nucleus = p[2 + y->n_onset];
    y->n_coda = p[3 + y->n_onset];
    y->coda = p + 4 + y->n_onset;
}

/* walks the records; off (DFX_MAX_SYL entries) gets their offsets, or 0 just to check */
static int dfx_scan(const uint8_t *data, uint32_t len, uint16_t *off)
{
    uint32_t i = 0, n = 0, k;
    if (len > DFX_MAX_LYRIC) return 0;
    while (i < len) {
        uint32_t no, nc;
        if (n >= DFX_MAX_SYL || i + 4 > len) return 0;
        no = data[i + 1];
        if (no > DFX_MAX_PH || i + 4 + no > len) return 0;
        nc = data[i + 3 + no];
        if (nc > DFX_MAX_PH || i + 4 + no + nc > len) return 0;
        for (k = 0; k < no; k++) if (data[i + 2 + k] == 0 || data[i + 2 + k] >= DFX_NPHONE) return 0;
        for (k = 0; k < nc; k++) if (data[i + 4 + no + k] == 0 || data[i + 4 + no + k] >= DFX_NPHONE) return 0;
        if (data[i + 2 + no] >= DFX_NPHONE) return 0;
        if (off) off[n] = (uint16_t)i;
        n++;
        i += 4 + no + nc;
    }
    return (int)n;
}

int dfx_lyrics_valid(const uint8_t *data, uint32_t len) { return dfx_scan(data, len, 0); }

int dfx_set_lyrics(dfx_singer_t *s, const uint8_t *data, uint32_t len)
{
    const int n = dfx_scan(data, len, 0);                 /* check it all before taking it */
    if (n == 0) return 0;
    dfx_scan(data, len, s->syl_off);
    s->lyric = data;
    s->nsyl = (uint16_t)n;
    dfx_restart(s);
    return n;
}

void dfx_restart(dfx_singer_t *s)
{
    s->next_syl = 0;
    s->cur_syl = -1;
    s->viz_syl = -1;
}

/* the next syllable to sing: >= 0, -2 a rest (silent note), -1 nothing */
static int dfx_pick_next(dfx_singer_t *s)
{
    dfx_syl_t y;
    if (s->nsyl == 0) return -1;
    if (s->next_syl >= s->nsyl) {
        if (!s->p.loop) return -1;
        s->next_syl = 0;
    }
    dfx_syl_at(s, s->next_syl, &y);
    s->next_syl++;
    return (y.flags & DFX_REST) ? -2 : s->next_syl - 1;
}

/* ------------------------------------------------- syllables, notes --- */
static void dfx_release_current(dfx_singer_t *s)
{
    int i;
    dfx_syl_t y;
    if (!(s->seg.dur < 0 && s->seg.nucleus)) {
        for (i = s->q_count - 1; i >= 0; i--) {           /* a very short note still gets a vowel */
            dfx_seg_t *q = dfx_q_at(s, i);
            if (q->dur < 0 && q->nucleus) { q->dur = dfx_ms_to_samples(70); break; }
        }
    }
    if (s->cur_syl >= 0) {
        dfx_syl_at(s, s->cur_syl, &y);
        if (DFX_PHONES[y.nucleus].off) {                  /* diphthong: glide to the second vowel */
            dfx_seg_t g;
            dfx_frame_t f;
            dfx_frame_for(&f, DFX_PHONES[y.nucleus].off);
            dfx_mk(&g, &f, 90 << 8, 40, 20, DFX_PHONES[y.nucleus].off);
            dfx_push_seg(s, &g);
        }
        for (i = 0; i < y.n_coda; i++) dfx_push_consonant(s, y.coda[i], y.nucleus, 0, 0);
    }
    dfx_push_silence(s);
    s->cur_syl = -1;
}

static void dfx_start_syllable(dfx_singer_t *s, int idx, uint8_t note, uint8_t vel)
{
    dfx_syl_t y;
    int32_t backlog = 0;
    int i, first, merge;
    dfx_seg_t *tail = 0, v;
    dfx_syl_at(s, idx, &y);
    if (s->cur_syl >= 0) dfx_release_current(s);

    for (i = 0; i < s->q_count; i++) if (dfx_q_at(s, i)->dur > 0) backlog += dfx_q_at(s, i)->dur;
    if (backlog > dfx_ms_to_samples(80))                      /* squeeze an old coda to stay on time */
        for (i = 0; i < s->q_count; i++) {
            dfx_seg_t *q = dfx_q_at(s, i);
            if (q->dur > 0) q->dur = q->dur / 2 > dfx_ms_to_samples(5) ? q->dur / 2 : dfx_ms_to_samples(5);
        }
    if (s->q_count && dfx_q_at(s, s->q_count - 1)->dur < 0) s->q_count--;   /* drop the silence hold */

    /* "is the": a fricative into a fricative would be one long hiss; halve both */
    if (s->q_count) tail = dfx_q_at(s, s->q_count - 1);
    else if (s->seg.dur > 0) tail = &s->seg;
    merge = tail && y.n_onset && dfx_is_fric(y.onset[0]) && dfx_is_fric(tail->ph);
    if (merge) tail->dur = tail->dur / 2 > dfx_ms_to_samples(25) ? tail->dur / 2 : dfx_ms_to_samples(25);
    first = s->q_count;
    for (i = 0; i < y.n_onset; i++) dfx_push_consonant(s, y.onset[i], y.nucleus, 1, i + 1 == y.n_onset);
    if (merge && s->q_count > first) {
        dfx_seg_t *q = dfx_q_at(s, first);
        q->dur = q->dur / 2 > dfx_ms_to_samples(25) ? q->dur / 2 : dfx_ms_to_samples(25);
    }

    memset(&v, 0, sizeof v);
    dfx_frame_for(&v.tgt, y.nucleus);
    v.dur = -1;
    v.ftau = y.n_onset ? 35 : 15;
    v.atau = y.n_onset ? 18 : 12;
    v.ph = y.nucleus;
    v.nucleus = 1;
    dfx_push_seg(s, &v);

    s->inc_target = DFX_NOTE_INC[note & 127];
    if (!(s->cur.av > 1638 || s->q_count > 1) || s->p.glide_ms <= 0) s->inc = s->inc_target;
    s->vel_target = 11469 + (int32_t)(21299 * (int32_t)(vel > 127 ? 127 : vel) / 127);   /* 0.35 + 0.65 v */
    s->cur_syl = (int16_t)idx;
    s->viz_syl = (int16_t)idx;
}

static void dfx_held_remove(dfx_singer_t *s, uint8_t note)
{
    int i, j;
    for (i = 0; i < s->nheld; i++)
        if (s->held[i] == note) {
            for (j = i; j < s->nheld - 1; j++) s->held[j] = s->held[j + 1];
            s->nheld--;
            return;
        }
}

void dfx_note_on(dfx_singer_t *s, uint8_t note, uint8_t vel)
{
    int idx;
    dfx_held_remove(s, note);
    if (s->nheld < sizeof s->held) s->held[s->nheld++] = note;
    idx = dfx_pick_next(s);
    if (idx == -2) {                                      /* a rest: silence */
        if (s->cur_syl >= 0) dfx_release_current(s);
        return;
    }
    if (idx >= 0) dfx_start_syllable(s, idx, note, vel);
}

void dfx_note_off(dfx_singer_t *s, uint8_t note)
{
    const int was_top = s->nheld && s->held[s->nheld - 1] == note;
    dfx_held_remove(s, note);
    if (!was_top) return;
    if (s->nheld) {                                       /* last-note priority: glide back */
        s->inc_target = DFX_NOTE_INC[s->held[s->nheld - 1] & 127];
        return;
    }
    if (s->cur_syl >= 0) dfx_release_current(s);
}

void dfx_all_off(dfx_singer_t *s)
{
    s->nheld = 0;
    if (s->cur_syl >= 0) dfx_release_current(s);
}

/* ------------------------------------------------------------ dfx_control --- */
static void dfx_advance(dfx_singer_t *s, int32_t n)
{
    s->seg_elapsed += n;
    for (;;) {
        const int timed_out = s->seg.dur >= 0 && s->seg_elapsed >= s->seg.dur;
        const int hold_ended = s->seg.dur < 0 && s->q_count > 0;
        if (!(timed_out || hold_ended)) break;
        if (s->q_count == 0) {                            /* ran out: hold silence */
            s->seg.tgt.av = s->seg.tgt.ah = s->seg.tgt.af = 0;
            s->seg.dur = -1;
            s->seg.atau = 20;
            s->seg.ftau = 60;
            s->seg.nucleus = 0;
            s->seg.ph = 0;
            s->seg_elapsed = 0;
            break;
        }
        s->seg_elapsed = timed_out ? s->seg_elapsed - s->seg.dur : 0;
        s->seg = *dfx_q_at(s, 0);
        s->q_head = (uint8_t)((s->q_head + 1) % DFX_QUEUE);
        s->q_count--;
        if (s->seg.nucleus) s->nucleus_age = 0;
    }
}

/* x += (target - x) * (1 - exp(-dt / tau)) */
static int32_t dfx_smooth(int32_t x, int32_t t, uint8_t tau)
{
    const int32_t k = 32767 - DFX_TAU_Q15[tau ? tau : 1];
    return x + (int32_t)(((int64_t)(t - x) * k) >> 15);
}

static void dfx_control(dfx_singer_t *s)
{
    const dfx_frame_t *t = &s->seg.tgt;
    dfx_frame_t *c = &s->cur;
    const uint8_t ft = s->seg.ftau, at = s->seg.atau;
    const int32_t sh = s->p.shift;
    int i;
    for (i = 0; i < 5; i++) {
        c->F[i] = dfx_smooth(c->F[i], t->F[i], ft);
        c->B[i] = dfx_smooth(c->B[i], t->B[i], ft);
    }
    c->fnz = dfx_smooth(c->fnz, t->fnz, ft);
    c->fr1f = dfx_smooth(c->fr1f, t->fr1f, ft);
    c->fr1b = dfx_smooth(c->fr1b, t->fr1b, ft);
    c->fr2f = dfx_smooth(c->fr2f, t->fr2f, ft);
    c->fr2b = dfx_smooth(c->fr2b, t->fr2b, ft);
    c->fr1a = dfx_smooth(c->fr1a, t->fr1a, at);
    c->fr2a = dfx_smooth(c->fr2a, t->fr2a, at);
    c->byp = dfx_smooth(c->byp, t->byp, at);
    c->av = dfx_smooth(c->av, t->av, at);
    c->ah = dfx_smooth(c->ah, t->ah, at);
    c->af = dfx_smooth(c->af, t->af, at);
    s->vel_gain = dfx_smooth(s->vel_gain, s->vel_target, at);

    /* glide: the increment moves exponentially (its log would be exact; this is close for small steps) */
    {
        int32_t g = s->p.glide_ms * 4 / 10;
        if (g < 1) s->inc = s->inc_target;
        else {
            const int32_t k = 32767 - DFX_TAU_Q15[g > 255 ? 255 : g];
            s->inc += (uint32_t)(((int64_t)((int64_t)s->inc_target - (int64_t)s->inc) * k) >> 15);
        }
    }

    /* the filters at this tick's formants (FORMANT scales F, and B by about its square root) */
    {
        const int32_t bsh = (256 + sh) / 2;               /* ~ sqrt for 0.7 .. 1.5 */
        for (i = 0; i < 5; i++)
            dfx_res_set(&s->casc[i], (int32_t)(((int64_t)c->F[i] * sh) >> 8), (int32_t)(((int64_t)c->B[i] * bsh) >> 8));
        dfx_res_set(&s->npole, (270 * 16 * sh) >> 8, 100 << 4);
        dfx_antires_set(&s->nzero, (int32_t)(((int64_t)c->fnz * sh) >> 8), 100 << 4);
        dfx_res_set(&s->fr1, (int32_t)(((int64_t)c->fr1f * sh) >> 8), c->fr1b);
        dfx_res_set(&s->fr2, (int32_t)(((int64_t)c->fr2f * sh) >> 8), c->fr2b);
    }
    /* source tilt: a dfx_pole at 4500 - 2500 * VOICE Hz, coef = exp(-2 pi f / FS), Q15 (a cubic is close here) */
    {
        const int32_t hz = 4500 - 2500 * s->p.source / 256;
        const int32_t x = (int32_t)((int64_t)hz * 205887 / DFX_FS);   /* 2 pi f / FS, Q15 */
        const int32_t x2 = (int32_t)(((int64_t)x * x) >> 15), x3 = (int32_t)(((int64_t)x2 * x) >> 15);
        s->tilt_coef = 32768 - x + x2 / 2 - x3 / 6;
    }
    s->viz_ph = s->seg.ph;
    if (s->nucleus_age < 0xFFFF) s->nucleus_age++;
}

/* ------------------------------------------------------------ render --- */
/* band-limited step correction, t and dt Q16 (of a period), result Q16 */
static int32_t dfx_blep(int32_t t, int32_t dt)
{
    if (dt <= 0) return 0;
    if (t < dt) {
        const int32_t x = (int32_t)(((int64_t)t << 16) / dt);
        return 2 * x - (int32_t)(((int64_t)x * x) >> 16) - 65536;
    }
    if (t > 65536 - dt) {
        const int32_t x = (int32_t)(((int64_t)(t - 65536) << 16) / dt);
        return (int32_t)(((int64_t)x * x) >> 16) + 2 * x + 65536;
    }
    return 0;
}

/* tanh-like soft clip, Q12 in, Q12 out (x (27 + x^2) / (27 + 9 x^2)) */
static int32_t dfx_soft_clip(int32_t x)
{
    int64_t x2;
    if (x > 3 * 4096) return 4096;
    if (x < -3 * 4096) return -4096;
    x2 = ((int64_t)x * x) >> 12;
    return (int32_t)(((int64_t)x * (27 * 4096 + x2)) / (27 * 4096 + 9 * x2));
}

static uint32_t dfx_rnd(dfx_singer_t *s) { return s->rng = s->rng * 1664525u + 1013904223u; }

void dfx_render(dfx_singer_t *s, int32_t *out, uint32_t n)
{
    const int32_t src = s->p.source, breath = s->p.breath;
    uint32_t i = 0;
    while (i < n) {
        uint32_t k, chunk;
        uint32_t inc;
        if (s->ctl_left <= 0) {
            dfx_advance(s, DFX_CTL);
            dfx_control(s);
            s->ctl_left = DFX_CTL;
        }
        chunk = n - i < (uint32_t)s->ctl_left ? n - i : (uint32_t)s->ctl_left;

        /* vibrato (fades in over ~0.35 s of the vowel) and a little jitter */
        s->vib_phase += (uint32_t)s->p.vib_rate16 * 6087u * chunk;               /* 2^32 / (16 FS) per sample */
        {
            const int32_t fade = dfx_clampi((int32_t)s->nucleus_age * 256 / 965, 0, 256);   /* 965 ticks = 0.35 s */
            const uint32_t idx = s->vib_phase >> 21;                                     /* 0..2047 over a cycle */
            const int32_t cosv = (idx < 1024 ? DFX_COS_Q30[idx] : -DFX_COS_Q30[idx - 1024]) >> 15;   /* +-32768 */
            const int32_t cents = (int32_t)s->p.vib_cents * fade / 256 * cosv / 32768 + ((int32_t)(dfx_rnd(s) >> 29) - 4);
            /* 2^(c/1200) ~ 1 + c * 0.000578: Q16 */
            inc = (uint32_t)(((int64_t)s->inc * (65536 + cents * 38)) >> 16);
        }
        {
            const int32_t dt = (int32_t)(inc >> 16) < 29491 ? (int32_t)(inc >> 16) : 29491;   /* <= 0.45 */
            const int32_t av = (int32_t)(((int64_t)s->cur.av * s->vel_gain) >> 15);
            for (k = 0; k < chunk; k++) {
                int32_t t, saw, glot, voice, x, n1, n2, asp, fn, fric, y;
                s->phase += inc;
                t = (int32_t)(s->phase >> 16);                                 /* Q16 */

                saw = 65536 - 2 * t + dfx_blep(t, dt);                             /* falling saw */
                glot = 0;
                if (t < DFX_OQ_Q16) {
                    const int32_t xq = (int32_t)(((int64_t)t * 109227) >> 16);  /* t / 0.6, Q16 (no divide) */
                    glot = 2 * xq - (int32_t)((3 * (int64_t)xq * xq) >> 16);
                }
                {
                    int32_t tc = t - DFX_OQ_Q16;
                    if (tc < 0) tc += 65536;
                    glot += dfx_blep(tc, dt) / 2;
                }
                glot = glot * 8 / 5;                                           /* * 1.6 */
                voice = (int32_t)(((int64_t)(256 - src) * saw * 7 / 10 + (int64_t)src * glot) >> 8);   /* Q16 */
                voice = (int32_t)(((int64_t)voice * av) >> 19);                 /* Q16 * Q15 -> Q12 */

                n1 = (int32_t)(dfx_rnd(s) >> 20) - 2048;                           /* +-0.5, Q12 */
                n2 = (int32_t)(dfx_rnd(s) >> 20) - 2048;
                {
                    int32_t bt = (int32_t)(((int64_t)breath * av) >> 10);           /* breath * 0.25 * voicing */
                    if (t >= DFX_OQ_Q16) bt = bt * 35 / 100;                             /* louder while the glottis is open */
                    asp = (int32_t)(((int64_t)n1 * 2 * (s->cur.ah + bt)) >> 15);
                }

                x = voice + asp / 2;
                s->tilt += (int32_t)(((int64_t)(x - s->tilt) * (32768 - s->tilt_coef)) >> 15);
                x = s->tilt + (int32_t)(((int64_t)(x - s->tilt) * 11469) >> 15);  /* + 0.35 of the highs */

                x = dfx_res_run(&s->npole, x);
                x = dfx_antires_run(&s->nzero, x);
                x = dfx_res_run(&s->casc[4], x);
                x = dfx_res_run(&s->casc[3], x);
                x = dfx_res_run(&s->casc[2], x);
                x = dfx_res_run(&s->casc[1], x);
                x = dfx_res_run(&s->casc[0], x);

                fn = (int32_t)(((int64_t)n2 * 2 * s->cur.af) >> 15);
                fric = (int32_t)(((int64_t)dfx_res_run(&s->fr1, fn) * s->cur.fr1a + (int64_t)dfx_res_run(&s->fr2, fn) * s->cur.fr2a +
                                  (int64_t)fn * s->cur.byp) >> 15);
                s->fric_lp = fric + (int32_t)(((int64_t)(s->fric_lp - fric) * 12090) >> 15);   /* exp(-2 pi 7 kHz / FS): a gentle high cut */

                y = (int32_t)(((int64_t)x * 1843 + (int64_t)s->fric_lp * 655) >> 12);   /* 0.45 x + 0.16 fric */
                y = (int32_t)(((int64_t)y * s->p.gain) >> 8);
                out[i + k] += dfx_soft_clip(y) * 8;                                         /* Q12 -> Q15 */
            }
        }
        i += chunk;
        s->ctl_left -= (int32_t)chunk;
    }
}

void dfx_set_inc(dfx_singer_t *s, uint32_t inc) { s->inc_target = inc; }

int dfx_silent(const dfx_singer_t *s)
{
    return s->q_count == 0 && s->seg.dur < 0 && !s->seg.nucleus && s->nheld == 0 && s->cur.av < 33 && s->cur.af < 33 &&
           s->cur.ah < 33;
}

/* -------------------------------------------------------------- setup --- */
void dfx_default_params(dfx_params_t *p)
{
    p->source = 128;
    p->breath = 38;
    p->shift = 256;
    p->speed = 256;
    p->glide_ms = 60;
    p->vib_cents = 25;
    p->vib_rate16 = 88;
    p->gain = 179;      /* 0.7: the plugin's default VOLUME (-6 dB) times its 1.4 */
    p->loop = 1;
}

void dfx_init(dfx_singer_t *s)
{
    memset(s, 0, sizeof *s);
    dfx_default_params(&s->p);
    dfx_frame_for(&s->cur, 0);
    s->seg.tgt = s->cur;
    s->seg.dur = -1;
    s->rng = 22222u;
    s->cur_syl = -1;
    s->viz_syl = -1;
    s->inc = s->inc_target = DFX_NOTE_INC[60];
    s->tilt_coef = 20200;                                   /* dfx_control() sets it from VOICE each tick */
}
