#include "../../web/emu/felucca_web.c"
#include <math.h>
static double run(uint32_t eng)
{
    double s = 0; uint32_t n = 0, i, k;
    host_preset(&trk[0], eng, 0);
    trk[0].p[P_VOICE] = V_MONO;
    for (i = 0; i < 6; i++) {
        web_midi(0x90, 60 + i * 2, 100);
        for (k = 0; k < 400 * 44; k += 128) { web_render(128); for (uint32_t j = 0; j < 128; j++) { s += out_l[j] * out_l[j]; n++; } }
        web_midi(0x80, 60 + i * 2, 0);
        for (k = 0; k < 100 * 44; k += 128) web_render(128);
    }
    return 10 * log10(s / n + 1e-12);
}
int main(void)
{
    web_nor_erase(); web_boot();
    for (int k = 0; k < 600 * 44; k += 128) web_render(128);
    printf("VOICE (eng 5) %.1f dB rms\n", run(5));
    printf("ANALOG (eng 0) %.1f dB rms\n", run(0));
    printf("DAISY (eng 15) %.1f dB rms\n", run(ENGI_DAISY));
    return 0;
}
