#ifdef WIN32
#include <mmsystem.h>



typedef struct {
    HWAVEIN     wi;
    HWAVEOUT    wo;
    u8      *name;
    i32     samples;
    i32     channels;
    i32     bits;
    int     pos;
    void    *prev;
    void    *next;
} audio_file_t;



static  audio_file_t   *audio_file    = NULL;



int audio_close(audio_file_t *audiofile) {
    if(audiofile->wi) {
        waveInClose(audiofile->wi);
        audiofile->wi = 0;
    }
    if(audiofile->wo) {
        waveOutClose(audiofile->wo);
        audiofile->wo = 0;
    }
    return(0);
}



int audio_common(audio_file_t *audiofile) {
    WAVEFORMATEX    wf;

    if(audiofile->wi && audiofile->wo) return(0);

    memset(&wf, 0, sizeof(wf));
    wf.wFormatTag       = WAVE_FORMAT_PCM;
    wf.nChannels        = audiofile->channels;
    wf.nSamplesPerSec   = audiofile->samples;
    wf.wBitsPerSample   = audiofile->bits;
    wf.nBlockAlign      = wf.nChannels * (wf.wBitsPerSample / 8);
    wf.nAvgBytesPerSec  = wf.nBlockAlign * wf.nSamplesPerSec;
    wf.cbSize           = sizeof(wf);

    if(!waveInGetNumDevs()) return(-1);
    if(waveInOpen(&audiofile->wi,  WAVE_MAPPER, &wf, 0, 0, WAVE_FORMAT_DIRECT) != MMSYSERR_NOERROR) return(-1);

    if(!waveOutGetNumDevs()) return(-1);
    if(waveOutOpen(&audiofile->wo, WAVE_MAPPER, &wf, 0, 0, WAVE_FORMAT_DIRECT) != MMSYSERR_NOERROR) return(-1);

    fprintf(stderr,
        "- opened audio device: %d/%d/%d\n",
        (i32)wf.nSamplesPerSec,
        (i32)wf.nChannels,
        (i32)wf.wBitsPerSample);
    return(0);
}



audio_file_t *audio_open(u8 *fname) {
    static  int init_audio = 0;
    audio_file_t    *audiofile  = NULL,
                    *audiofile_tmp;
    u8      name[256] = "",
            proto[16] = "";

    if(!strstr(fname, "://")) return(NULL);

    audiofile_tmp = calloc(1, sizeof(audio_file_t));
    if(!audiofile_tmp) STD_ERR(QUICKBMS_ERROR_MEMORY);

    audiofile_tmp->samples  = 44100;
    audiofile_tmp->channels = 2;
    audiofile_tmp->bits     = 16;

    sscanf(fname,
        "%10[^:]://%255[^,],%d,%d,%d",
        proto,
        name,
        &audiofile_tmp->samples,
        &audiofile_tmp->channels,
        &audiofile_tmp->bits);

    if(stricmp(proto, "audio") && stricmp(proto, "wave")) {
        FREE(audiofile_tmp);
        return(NULL);
    }
    if(!name[0]) {
        FREE(audiofile_tmp);
        return(NULL);
    }

    if(!enable_audio) {
        fprintf(stderr,
            "\n"
            "Error: the script uses the audio device, if you are SURE about the genuinity of\n"
            "       this script\n"
            "\n"
            "         you MUST use the -A or -audio option at command-line.\n"
            "\n"
            "       you MUST really sure about the script you are using and what you are\n"
            "       doing because this is NOT a feature for extracting files!\n");
        myexit(QUICKBMS_ERROR_EXTRA);
    }
    if(!init_audio) {
        init_audio = 1;
    }

    audiofile_tmp->name = mystrdup_simple(name);

    for(audiofile = audio_file; audiofile; audiofile = audiofile->next) {
        if(
            !stricmp(audiofile->name, audiofile_tmp->name) &&
            (audiofile->channels == audiofile_tmp->channels) &&
            (audiofile->samples  == audiofile_tmp->samples) &&
            (audiofile->bits     == audiofile_tmp->bits)
        ) {
            FREE(audiofile_tmp->name);
            FREE(audiofile_tmp);
            audiofile_tmp = NULL;
            break;
        }
    }
    if(!audiofile) {
        if(!audio_file) {
            audio_file = audiofile_tmp;
            audiofile = audio_file;
        } else {
            // get the last element
            for(audiofile = audio_file;; audiofile = audiofile->next) {
                if(audiofile->next) continue;
                audiofile->next = audiofile_tmp;
                audiofile_tmp->prev = audiofile;
                audiofile = audiofile_tmp;
                break;
            }
        }
    }

    audio_common(audiofile);
    return(audiofile);
}



int audio_read(audio_file_t *audiofile, u8 *data, int size) {
    WAVEHDR wh;
    MMTIME  mt;
    int     ret = -1;

    audio_common(audiofile);

    memset(&wh, 0, sizeof(wh));
    wh.dwBufferLength   = size;
    wh.lpData           = data;

    if(waveInPrepareHeader(audiofile->wi, &wh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) goto quit;
    if(waveInAddBuffer(audiofile->wi,     &wh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) goto quit;
    if(waveInStart(audiofile->wi) != MMSYSERR_NOERROR) goto quit;

    mt.wType = TIME_BYTES;
    do {
        if(waveInGetPosition(audiofile->wi, &mt, sizeof(mt)) != MMSYSERR_NOERROR) break;
        Sleep(50);  // do NOT decrease it!
    } while(mt.u.cb < size);
    ret = mt.u.cb;
    audiofile->pos += ret;
quit:
    waveInReset(audiofile->wi);
    waveInUnprepareHeader(audiofile->wi, &wh, sizeof(WAVEHDR));
    return(ret);
}



int audio_write(audio_file_t *audiofile, u8 *data, int size) {
    WAVEHDR wh;
    int     ret = -1;

    audio_common(audiofile);

    memset(&wh, 0, sizeof(wh));
    wh.dwBufferLength   = size;
    wh.lpData           = data;

    if(waveOutPrepareHeader(audiofile->wo, &wh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) goto quit;
    //if(waveOutAddBuffer(audiofile->wo,     &wh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) goto quit;
    //if(waveOutStart(audiofile->wo) != MMSYSERR_NOERROR) goto quit;
    waveOutWrite(audiofile->wo, &wh, sizeof(WAVEHDR));

    // not suggested because doesn't terminate with odd sizes or something similar (like 9)
    /*
    MMTIME  mt;
    mt.wType = TIME_BYTES;
    do {
        if(waveOutGetPosition(audiofile->wo, &mt, sizeof(mt)) != MMSYSERR_NOERROR) break;
        Sleep(50);
    } while(mt.u.cb < size);
    ret = mt.u.cb;
    */

    while(waveOutUnprepareHeader(audiofile->wo, &wh, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) {
        Sleep(50);  // do NOT decrease it!
    }
    ret = size;
    audiofile->pos += ret;
quit:
    waveOutReset(audiofile->wo);
    waveOutUnprepareHeader(audiofile->wo, &wh, sizeof(WAVEHDR));
    return(ret);
}



#else

typedef struct {
    u8      *name;
    i32     samples;
    i32     channels;
    i32     bits;
    int     pos;
} audio_file_t;
static  audio_file_t   *audio_file    = NULL;
audio_file_t *audio_open(u8 *fname) { return(NULL); }
int audio_read(audio_file_t *audiofile, u8 *data, int size) { return(-1); }
int audio_write(audio_file_t *audiofile, u8 *data, int size) { return(-1); }
int audio_close(audio_file_t *audiofile) { return(-1); }

#endif
