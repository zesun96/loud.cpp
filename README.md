# loud.cpp

Whisper.cpp with diarization

## Building

See [BUILDING.md](docs/BUILDING.md)

## Normalize wav file

```console
ffmpeg -i file.wav -ar 16000 -ac 1 -c:a pcm_s16le normal.wav
```
