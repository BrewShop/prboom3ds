#include "Mixer_3ds.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>

#define MIXER_CHANNEL_NUM 31

int MixerHardware3DS::init() {
	Result ret = 0;
	m_bufferSize = (1L<<16);

	m_soundBuffer = (byte *)linearAlloc(m_bufferSize);
	m_soundBufferPHY = osConvertVirtToPhys(m_soundBuffer);
	m_bufferPos = 0;
	m_soundPos = 0;
	m_lastPos = 0;
	clear();

	ret = csndPlaySound(MIXER_CHANNEL_NUM, SOUND_REPEAT | SOUND_FORMAT_8BIT, m_speed, 1.0f, 0.0f, (u32*)m_soundBuffer, (u32*)m_soundBuffer, m_bufferSize);
	if (ret != 0) {
		printf("mus init failed\n");
	}
	//try to start stalled channels
	u8 playing = 0;
	csndIsPlaying(MIXER_CHANNEL_NUM, &playing);
	if (playing == 0) {
		CSND_SetPlayState(MIXER_CHANNEL_NUM, 1);
	}
	//flush csnd command buffers
	csndExecCmds(true);
	m_start = svcGetSystemTick();

	return 0;
}

void MixerHardware3DS::clear() {
	memset(m_soundBuffer, 0, m_bufferSize);
	GSPGPU_FlushDataCache(m_soundBuffer, m_bufferSize);
}

void MixerHardware3DS::shutdown() {
	flush();
	CSND_SetPlayState(MIXER_CHANNEL_NUM, 0);
	csndExecCmds(true);
}

void MixerHardware3DS::flush() {
}

byte* MixerHardware3DS::buffer() {
	return m_soundBuffer;
}

#define TICKS_PER_SEC 268123480.0
#define TICKS_PER_SEC_LL 268111856LL

u64 MixerHardware3DS::samplepos() {
	u64 delta = (svcGetSystemTick() - m_start);
	u64 samples = delta * m_speed / TICKS_PER_SEC_LL;

	m_soundPos = samples;

	//printf("%2d %8d %10lld %10lld\n", m_channel, m_speed, speed, delta);

	return m_soundPos;
}

void MixerHardware3DS::update(int *pAudioData, int count) {
}

void MixerHardware3DS::update(short *pAudioData, int count)
{
	int volume = 1;
	int mask = m_bufferSize - 1;

	if (m_bufferPos < m_soundPos) {
		m_bufferPos = m_soundPos;
	}

	short *p = pAudioData;
	byte *outp = m_soundBuffer;
	int val;
	int pos = m_bufferPos & mask;
	m_bufferPos += count;
	while (count--) {
		val = (*p * volume) >> 0;
		p += 1;//step;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < (short)0x8000)
			val = (short)0x8000;
		outp[pos] = val>>8;
		pos = (pos + 1) & mask;
	}
	GSPGPU_FlushDataCache(m_soundBuffer, m_bufferSize);
}

#if 1
MixerHardware3DS g_mixer(32728, 1);

extern "C" void mixer_update(short *pAudioData, int count) {
	g_mixer.update(pAudioData, count);
}

extern "C" int mixer_pos() {
	return g_mixer.samplepos();
}

extern "C" int mixer_init() {
	return g_mixer.init();
}

extern "C" void mixer_exit() {
	g_mixer.shutdown();
}

extern "C" void mixer_clear() {
	g_mixer.clear();
}

#endif