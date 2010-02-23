REM Start qemu on windows.
REM tested with qemu-0.10.6-windows

@ECHO OFF

REM SDL_VIDEODRIVER=directx is faster than windib. But keyboard cannot work well.
SET SDL_VIDEODRIVER=windib

REM SDL_AUDIODRIVER=waveout or dsound can be used. Only if QEMU_AUDIO_DRV=sdl.
SET SDL_AUDIODRIVER=dsound

REM QEMU_AUDIO_DRV=dsound or fmod or sdl or none can be used. See qemu -audio-help.
SET QEMU_AUDIO_DRV=dsound

REM QEMU_AUDIO_LOG_TO_MONITOR=1 displays log messages in QEMU monitor.
SET QEMU_AUDIO_LOG_TO_MONITOR=0

REM PCI-based PC(default): -M pc 
REM ISA-based PC         : -M isapc
REM -M isapc is added for NE2000 ISA card.

REM the -L argument should point to the qemu libraries

set arguments=-L . -vga std -no-kqemu -M isapc -net nic,model=ne2k_isa -net user -redir udp:8080::8080 -redir udp:8081::8081 -redir tcp:8080::8080 -redir tcp:8081::8081 -boot d -cdrom image.iso

qemu.exe %* %arguments%