// WINMMWRP

#include "Common.hpp"
#include "Loader.hpp"

typedef unsigned int (*mM)(unsigned int, unsigned int, DWORD_PTR, DWORD_PTR, DWORD_PTR);

static OmniMIDI::Loader *ldr = nullptr;

extern "C" {
	EXPORT BOOL APIENTRY DllMain(HMODULE hModule,
								 DWORD ul_reason_for_call,
								 LPVOID lpReserved)
	{
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
			if (ldr == nullptr)
			{
				ldr = new OmniMIDI::Loader;

				if (ldr->LoadWinMMModule())
				{
					return TRUE;
				}
			}
			break;

		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
		}

		return FALSE;
	}

	EXPORT LRESULT WINAPI WRP_DefDriverProc(DWORD_PTR dwDId, HDRVR drv, UINT msg, LONG lP1, LONG lP2)
	{
		return MMDefDriverProc(dwDId, drv, msg, lP1, lP2);
	}

	EXPORT UINT WINAPI WRP_midiOutGetNumDevs()
	{
		return MMmidiOutGetNumDevs();
	}

	EXPORT MMRESULT WINAPI WRP_midiOutGetDevCapsW(UINT_PTR uDeviceID, LPMIDIOUTCAPSW lpCaps, UINT uSize)
	{
		auto v = uDeviceID;

		auto r = MMmidiOutGetDevCapsW(v, lpCaps, uSize);
		return r;
	}

	EXPORT MMRESULT WINAPI WRP_midiOutGetDevCapsA(UINT_PTR uDeviceID, LPMIDIOUTCAPSA lpCaps, UINT uSize)
	{
		MIDIOUTCAPSA myCaps;
		MIDIOUTCAPSW myCapsW;
		UINT ret;

		if (!lpCaps || uSize != sizeof(MIDIOUTCAPSA))
			return MMSYSERR_INVALPARAM;

		ret = WRP_midiOutGetDevCapsW(uDeviceID, &myCapsW, sizeof(MIDIOUTCAPSW));

		if (ret == MMSYSERR_NOERROR)
		{
			// Assign values
			myCaps.wMid = myCapsW.wMid;
			myCaps.wPid = myCapsW.wPid;
			myCaps.wVoices = myCapsW.wVoices;
			myCaps.wNotes = myCapsW.wNotes;
			myCaps.wTechnology = myCapsW.wTechnology;
			myCaps.wChannelMask = myCapsW.wChannelMask;
			myCaps.dwSupport = myCapsW.dwSupport;

			// Convert wchar_t* to char*
			wcstombs(myCaps.szPname, myCapsW.szPname, MAXPNAMELEN);

			// Copy values to pointer, and return 0
			memcpy(lpCaps, &myCaps, fmin(uSize, sizeof(myCaps)));
		}

		return ret;
	}

	EXPORT MMRESULT WINAPI WRP_midiOutOpen(LPHMIDIOUT lphmo, UINT uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwCallbackInstance, DWORD dwFlags)
	{
		return MMmidiOutOpen(lphmo, uDeviceID, dwCallback, dwCallbackInstance, dwFlags);
	}

	EXPORT MMRESULT WINAPI WRP_midiOutClose(HMIDIOUT hMidiOut)
	{
		return MMmidiOutClose(hMidiOut);
	}

	EXPORT MMRESULT WINAPI WRP_midiOutReset(HMIDIOUT hMidiOut)
	{
		auto dev = (midi_device_internal_t *)hMidiOut;
		return dev->vtbl->modMessage(dev->id, MODM_RESET, dev->user_ptr, 0, 0);
	}

	EXPORT MMRESULT WINAPI WRP_midiOutPrepareHeader(HMIDIOUT hMidiOut, LPMIDIHDR lpMidiOutHdr, UINT uSize)
	{
		auto dev = (midi_device_internal_t *)hMidiOut;
		MMRESULT r = dev->vtbl->modMessage(dev->id, MODM_PREPARE, dev->user_ptr, (DWORD_PTR)lpMidiOutHdr, (DWORD_PTR)uSize);

		if (r == MMSYSERR_NOTSUPPORTED)
			r = MMSYSERR_NOERROR;

		if (r == MMSYSERR_NOERROR)
			lpMidiOutHdr->dwFlags |= MHDR_PREPARED;

		return r;
	}

	EXPORT MMRESULT WINAPI WRP_midiOutUnprepareHeader(HMIDIOUT hMidiOut, LPMIDIHDR lpMidiOutHdr, UINT uSize)
	{
		auto dev = (midi_device_internal_t *)hMidiOut;
		MMRESULT r = dev->vtbl->modMessage(dev->id, MODM_UNPREPARE, dev->user_ptr, (DWORD_PTR)lpMidiOutHdr, (DWORD_PTR)uSize);

		if (r == MMSYSERR_NOTSUPPORTED)
			r = MMSYSERR_NOERROR;

		if (r == MMSYSERR_NOERROR)
			lpMidiOutHdr->dwFlags &= ~MHDR_PREPARED;

		return r;
	}

	EXPORT MMRESULT WINAPI WRP_midiOutShortMsg(HMIDIOUT hMidiOut, DWORD dwMsg)
	{
		auto dev = (midi_device_internal_t *)hMidiOut;
		return dev->vtbl->modMessage(dev->id, MODM_DATA, dev->user_ptr, (DWORD_PTR)dwMsg, 0);
	}

	EXPORT MMRESULT WINAPI WRP_midiOutLongMsg(HMIDIOUT hMidiOut, LPMIDIHDR lpMidiOutHdr, UINT uSize)
	{
		auto dev = (midi_device_internal_t *)hMidiOut;
		return dev->vtbl->modMessage(dev->id, MODM_LONGDATA, dev->user_ptr, (DWORD_PTR)lpMidiOutHdr, (DWORD_PTR)uSize);
	}

	EXPORT MMRESULT WINAPI WRP_midiOutMessage(HMIDIOUT hMidiOut, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2)
	{
		auto dev = (midi_device_internal_t *)hMidiOut;
		return dev->vtbl->modMessage(dev->id, uMsg, dev->user_ptr, dw1, dw2);
	}

	EXPORT MMRESULT WINAPI WRP_midiOutSetVolume(HMIDIOUT hMidiOut, DWORD dwVolume)
	{
		auto dev = (midi_device_internal_t *)hMidiOut;
		return dev->vtbl->modMessage(dev->id, MODM_SETVOLUME, dev->user_ptr, (DWORD_PTR)dwVolume, 0);
	}

	EXPORT MMRESULT WINAPI WRP_midiOutGetVolume(HMIDIOUT hMidiOut, LPDWORD lpdwVolume)
	{
		auto dev = (midi_device_internal_t *)hMidiOut;
		return dev->vtbl->modMessage(dev->id, MODM_GETVOLUME, dev->user_ptr, (DWORD_PTR)lpdwVolume, 0);
	}

	EXPORT MMRESULT WINAPI WRP_midiOutGetID(HMIDIOUT hMidiOut, LPUINT puDeviceID)
	{
		if (!puDeviceID)
			return MMSYSERR_INVALPARAM;

		auto dev = (midi_device_internal_t *)hMidiOut;
		*puDeviceID = dev->id;
		return MMSYSERR_NOERROR;
	}

	EXPORT MMRESULT WINAPI WRP_midiOutCachePatches(HMIDIOUT hMidiOut, UINT wPatch, LPWORD lpPatchArray, UINT wFlags)
	{
		auto dev = (midi_device_internal_t *)hMidiOut;
		return dev->vtbl->modMessage(dev->id, MODM_CACHEPATCHES, dev->user_ptr, (DWORD_PTR)lpPatchArray, MAKELONG(wPatch, wFlags));
	}

	EXPORT MMRESULT WINAPI WRP_midiOutCacheDrumPatches(HMIDIOUT hMidiOut, UINT wPatch, LPWORD lpKeyArray, UINT wFlags)
	{
		auto dev = (midi_device_internal_t *)hMidiOut;
		return dev->vtbl->modMessage(dev->id, MODM_CACHEDRUMPATCHES, dev->user_ptr, (DWORD_PTR)lpKeyArray, MAKELONG(wPatch, wFlags));
	}

	EXPORT MMRESULT WINAPI WRP_midiStreamOpen(LPHMIDISTRM lphStream, LPUINT puDeviceID, DWORD cMidi, DWORD_PTR dwCallback, DWORD_PTR dwCallbackInstance, DWORD fdwOpen)
	{
		return MMmidiStreamOpen(lphStream, puDeviceID, cMidi, dwCallback, dwCallbackInstance, fdwOpen);
	}

	EXPORT MMRESULT WINAPI WRP_midiStreamClose(HMIDISTRM hStream)
	{
		return MMmidiStreamClose(hStream);
	}

	EXPORT MMRESULT WINAPI WRP_midiStreamOut(HMIDISTRM hStream, LPMIDIHDR lpMidiOutHdr, UINT uSize)
	{
		return MMmidiStreamOut(hStream, lpMidiOutHdr, uSize);
	}

	EXPORT MMRESULT WINAPI WRP_midiStreamPause(HMIDISTRM hStream)
	{
		return MMmidiStreamPause(hStream);
	}

	EXPORT MMRESULT WINAPI WRP_midiStreamRestart(HMIDISTRM hStream)
	{
		return MMmidiStreamRestart(hStream);
	}

	EXPORT MMRESULT WINAPI WRP_midiStreamStop(HMIDISTRM hStream)
	{
		return MMmidiStreamStop(hStream);
	}

	EXPORT MMRESULT WINAPI WRP_midiStreamProperty(HMIDISTRM hStream, LPBYTE lppropdata, DWORD dwProperty)
	{
		return MMmidiStreamProperty(hStream, lppropdata, dwProperty);
	}

	EXPORT MMRESULT WINAPI WRP_midiStreamPosition(HMIDISTRM hStream, LPMMTIME pmmt, UINT cbmmt)
	{
		return MMmidiStreamPosition(hStream, pmmt, cbmmt);
	}

	EXPORT DWORD WINAPI WRP_mixerGetNumDevs()
	{
#ifdef CODPATCH
		DWORD ret = MMmixerGetNumDevs();
		return ret > 15 ? 15 : ret;
#else
		return MMmixerGetNumDevs();
#endif
	}

	EXPORT DWORD WINAPI WRP_waveInGetNumDevs()
	{
#ifdef CODPATCH
		DWORD ret = MMwaveInGetNumDevs();
		return ret > 15 ? 15 : ret;
#else
		return MMwaveInGetNumDevs();
#endif
	}
}