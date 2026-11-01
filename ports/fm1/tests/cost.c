#include "../../web/emu/felucca_web.c"
#include <time.h>
static double secs(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static double run(int eng)
{
    uint32_t k;
    double t0;
    if (eng >= 0) { host_preset(&trk[0], (uint32_t)eng, 0); trk[0].p[P_VOICE] = V_MONO; trk[0].p[P_SUS] = 127; web_midi(0x90, 60, 100); }
    for (k = 0; k < 44100; k += 128) web_render(128);
    t0 = secs();
    for (k = 0; k < 120 * 44100; k += 128) web_render(128);
    t0 = secs() - t0;
    if (eng >= 0) web_midi(0x80, 60, 0);
    for (k = 0; k < 2 * 44100; k += 128) web_render(128);
    return t0;
}
int main(void)
{
    double idle, an, da;
    web_nor_erase(); web_boot();
    for (int k = 0; k < 600 * 44; k += 128) web_render(128);
    idle = run(-1); an = run(0); da = run(ENGI_DAISY);
    printf("idle %.3fs  ANALOG +%.3fs  DAISY +%.3fs  (DAISY / ANALOG voice = %.2f)\n", idle, an - idle, da - idle, (da - idle) / (an - idle));
    return 0;
}
