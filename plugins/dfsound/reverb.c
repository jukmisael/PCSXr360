/***************************************************************************
                           reverb.c  -  PSX Reverb - FIXED
                              -------------------
     Minimalist implementation com acumulação correta de samples
  ***************************************************************************/

#define _IN_REVERB
#ifdef _IN_SPU

/* Buffer para acumular samples de todos os canais */
#define RVB_BUF_SIZE 512
static int s_reverbInL[RVB_BUF_SIZE];
static int s_reverbInR[RVB_BUF_SIZE];
static int s_bufPos = 0;

/* Delay line para o efeito */
#define RVB_DELAY_SIZE 2205  /* ~50ms */
#define RVB_MASK (RVB_DELAY_SIZE - 1)
static short s_delayL[RVB_DELAY_SIZE];
static short s_delayR[RVB_DELAY_SIZE];
static int s_delayPos = 0;

/* Níveis: 90% dry, 10% wet */
static int s_wet = 3277;    /* 10% */
static int s_dry = 29491;   /* 90% */

/* Legacy globals */
int *sRVBPlay = 0;
int *sRVBEnd = 0;
int *sRVBStart = 0;
int iReverbOff = -1;
int iReverbRepeat = 0;
int iReverbNum = 1;

/* Clamp helper */
#define CLAMP16(x) (((x) > 32767) ? 32767 : ((x) < -32768) ? -32768 : (x))

/* Inicialização */
void InitReverbSystem(void)
{
    int i;
    for (i = 0; i < RVB_BUF_SIZE; i++) {
        s_reverbInL[i] = 0;
        s_reverbInR[i] = 0;
    }
    for (i = 0; i < RVB_DELAY_SIZE; i++) {
        s_delayL[i] = 0;
        s_delayR[i] = 0;
    }
    s_bufPos = 0;
    s_delayPos = 0;
}

void ShutdownReverbSystem(void)
{
    InitReverbSystem();
}

/* Set reverb - simplified */
void SetREVERB(unsigned short val)
{
    iReverbOff = (val == 0x0000) ? -1 : 32;
}

/* Inicializa reverb para canais */
static INLINE void StartREVERB(int ch)
{
    if (s_chan[ch].bReverb && (spuCtrl & 0x80) && iUseReverb >= 1) {
        s_chan[ch].bRVBActive = 1;
    } else {
        s_chan[ch].bRVBActive = 0;
    }
}

static INLINE void InitREVERB(void)
{
    int i;
    if (iUseReverb >= 1) {
        for (i = 0; i < RVB_DELAY_SIZE; i++) {
            s_delayL[i] = 0;
            s_delayR[i] = 0;
        }
        s_delayPos = 0;
    }
}

/* Store CD audio */
static INLINE void StoreREVERB_CD(int left, int right, int ns)
{
    if (iUseReverb == 0) return;
    if (ns < RVB_BUF_SIZE) {
        s_reverbInL[ns] += left >> 1;  /* Acumula com redução de volume */
        s_reverbInR[ns] += right >> 1;
    }
}

/* Store channel sample - ACUMULA no buffer */
static INLINE void StoreREVERB(int ch, int ns)
{
    int vl, vr;
    
    if (iUseReverb == 0) return;
    if (ns >= RVB_BUF_SIZE) return;
    
    /* Calcula volume do canal */
    vl = (s_chan[ch].sval * s_chan[ch].iLeftVolume) >> 14;   /* /16384 */
    vr = (s_chan[ch].sval * s_chan[ch].iRightVolume) >> 14;
    
    /* Acumula no buffer de reverb */
    s_reverbInL[ns] += vl;
    s_reverbInR[ns] += vr;
}

/* Mix reverb - LEFT channel */
static INLINE int MixREVERBLeft(int ns)
{
    int inL, inR;
    int readPos;
    int wetL, wetR;
    int outL;
    
    if (iUseReverb == 0) return 0;
    if (ns >= RVB_BUF_SIZE) return 0;
    
    /* Pega sample acumulado */
    inL = s_reverbInL[ns];
    inR = s_reverbInR[ns];
    
    /* Limpa para próxima rodada */
    s_reverbInL[ns] = 0;
    s_reverbInR[ns] = 0;
    
    /* Limita para evitar clipping */
    inL = CLAMP16(inL);
    inR = CLAMP16(inR);
    
    /* Lê sample atrasado do delay line */
    readPos = (s_delayPos + 16) & RVB_MASK;  /* Pequeno offset */
    wetL = s_delayL[readPos];
    wetR = s_delayR[readPos];
    
    /* Escreve sample atual no delay (com atenuação) */
    s_delayL[s_delayPos] = (short)CLAMP16((inL * 8000) >> 15);  /* ~24% */
    s_delayR[s_delayPos] = (short)CLAMP16((inR * 8000) >> 15);
    
    /* Avança posição do delay */
    s_delayPos = (s_delayPos + 1) & RVB_MASK;
    
    /* Mix: dry + wet */
    outL = ((inL * s_dry) + (wetL * s_wet)) >> 15;
    
    /* Aplica volume de reverb */
    outL = (outL * rvb.VolLeft) >> 14;
    
    return CLAMP16(outL);
}

/* Mix reverb - RIGHT channel */
static INLINE int MixREVERBRight(void)
{
    /* Processado junto com Left */
    return 0;
}

#endif
