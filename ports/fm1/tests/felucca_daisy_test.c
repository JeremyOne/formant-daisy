/* DAISY inside Felucca, without an FM-1: Felucca's browser-emulator build (web/emu/felucca_web.c, the
 * firmware against a simulated FM-1) compiled natively, with DAISY on track 1.
 *   - plays the Daisy Bell melody (Harry Dacre, 1892, public domain) into it by MIDI and checks each note
 *     sang the next syllable, at a sensible level, and that the voice ends after the last note
 *   - sends the ROBOT lyric as the SysEx upload a computer would, and checks the part switched to it
 *   - starts the sequencer and checks the lyric went back to its first syllable
 *   - writes what it played to OUT.wav
 * Build from a Felucca checkout with the DAISY patch applied (ports/fm1/felucca/apply.sh):
 *   cc -O2 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -Ifirmware/bsp/chip \
 *      -Ifirmware/bsp/boards/fm1 -o daisy_test felucca_daisy_test.c -lm
 *   ./daisy_test OUT.wav */
#include "../../web/emu/felucca_web.c"
#include <math.h>

static FILE *wav;
static uint32_t wav_n;
static float peak;
static double sumsq;

static void render(double ms)
{
    uint32_t n = (uint32_t)(ms * 44.1 + 0.5), k, i;
    for (k = 0; k < n; k += 128u) {
        uint32_t m = n - k < 128u ? n - k : 128u;
        web_render(m);
        for (i = 0; i < m; i++) {
            float l = out_l[i];
            int16_t q = (int16_t)(l > 1.0f ? 32767 : l < -1.0f ? -32767 : l * 32767.0f);
            if (wav) fwrite(&q, 2, 1, wav);
            wav_n++;
            if (fabsf(l) > peak) peak = fabsf(l);
            sumsq += (double)l * l;
        }
    }
}

static void wav_header(uint32_t n)
{
    uint32_t v;
    uint16_t h;
    fseek(wav, 0, SEEK_SET);
    fwrite("RIFF", 1, 4, wav); v = 36 + n * 2; fwrite(&v, 4, 1, wav);
    fwrite("WAVEfmt ", 1, 8, wav); v = 16; fwrite(&v, 4, 1, wav);
    h = 1; fwrite(&h, 2, 1, wav); fwrite(&h, 2, 1, wav);
    v = 44100; fwrite(&v, 4, 1, wav); v = 88200; fwrite(&v, 4, 1, wav);
    h = 2; fwrite(&h, 2, 1, wav); h = 16; fwrite(&h, 2, 1, wav);
    fwrite("data", 1, 4, wav); v = n * 2; fwrite(&v, 4, 1, wav);
}

static int fails;
static void check(int ok, const char *what)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) fails++;
}

int main(int argc, char **argv)
{
    /* the chorus's first two lines, 4/4 at 120 BPM: note, beats */
    static const uint8_t MEL[20][2] = {
        {72, 2}, {68, 2}, {65, 2}, {60, 2}, {61, 1}, {63, 1}, {65, 1}, {61, 1}, {65, 1}, {60, 3},
        {67, 2}, {72, 2}, {68, 2}, {65, 2}, {61, 1}, {63, 1}, {65, 1}, {67, 1}, {68, 1}, {67, 3},
    };
    daisy_part_t *d;
    uint32_t i, sang = 0, order_ok = 1;
    double rms;

    if (argc > 1 && (wav = fopen(argv[1], "wb")))
        wav_header(0);
    web_nor_erase();
    web_boot();
    render(600);
    host_preset(&trk[0], ENGI_DAISY, 0);      /* DAISY BELL preset on track 1 */
    trk[0].p[P_VOICE] = V_MONO;
    render(50);
    d = daisy_of(&trk[0]);
    check(trk[0].engine == ENGI_DAISY, "track 1 plays DAISY");

    peak = 0;
    sumsq = 0;
    wav_n = 0;
    for (i = 0; i < 20u; i++) {
        web_midi(0x90, MEL[i][0], 100);
        render(20);
        if (d->sg.viz_syl == (int16_t)i) sang++;
        else order_ok = 0;
        render(MEL[i][1] * 500.0 * 0.9 - 20);
        web_midi(0x80, MEL[i][0], 0);
        render(MEL[i][1] * 500.0 * 0.1);
    }
    rms = sqrt(sumsq / (wav_n ? wav_n : 1));
    printf("  20 notes: %u sang their syllable in order, peak %.2f, rms %.3f\n", sang, peak, rms);
    check(sang == 20 && order_ok, "each note sang the next syllable");
    check(peak > 0.05f && peak < 1.0f && rms > 0.01, "it was heard, without clipping");
    render(800);
    check(!trk[0].v[0].active, "the voice ended after the last note");

    /* a lyric upload, as fm1_encode --syx writes it (begin, data, end) */
    {
        static uint8_t frame[4 + 512];
        uint32_t len = sizeof DFX_LYRIC_ROBOT, k;
        const uint8_t begin[4] = {0x7D, 0x44, 0x46, 0x01}, end[4] = {0x7D, 0x44, 0x46, 0x03};
        daisy_sysex(begin, 4);
        for (k = 0; k < len; k += 512u) {
            uint32_t m = len - k < 512u ? len - k : 512u, j;
            frame[0] = 0x7D; frame[1] = 0x44; frame[2] = 0x46; frame[3] = 0x02;
            for (j = 0; j < m; j++) frame[4 + j] = DFX_LYRIC_ROBOT[k + j];
            daisy_sysex(frame, 4 + m);
        }
        daisy_sysex(end, 4);
        check(trk[0].p[P_E6] == 2 && daisy_user_len == len, "a SysEx upload became the USER lyric");
        web_midi(0x90, 60, 100);
        render(30);
        check(d->lyric == 2 && d->sg.nsyl == (uint16_t)dfx_lyrics_valid(DFX_LYRIC_ROBOT, len) && d->sg.viz_syl == 0,
              "the part sings the uploaded lyric from its first syllable");
        render(300);
        web_midi(0x80, 60, 0);
        render(400);
        /* a broken upload is refused, the lyric stays */
        daisy_sysex(begin, 4);
        frame[3] = 0x02; frame[4] = 1; frame[5] = 9;    /* a record cut short */
        daisy_sysex(frame, 6);
        daisy_sysex(end, 4);
        check(daisy_user_len == len, "a broken upload is refused");
    }

    /* the sequencer starting takes the lyric back to the top */
    web_midi(0x90, 62, 100);
    render(200);
    web_midi(0x80, 62, 0);
    render(300);
    check(d->sg.next_syl > 1, "(the lyric has moved on)");
    web_buttons(1u << B_PLAY);
    render(60);
    web_buttons(0);
    render(60);
    check(song.playing && d->sg.next_syl <= 1, "PLAY took the lyric back to its first syllable");

    if (wav) {
        wav_header(wav_n);
        fclose(wav);
    }
    printf(fails ? "%d FAILED\n" : "ALL PASS\n", fails);
    return fails ? 1 : 0;
}
