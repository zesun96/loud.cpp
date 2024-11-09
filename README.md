# loud.cpp

Whisper.cpp with diarization

## Features

-  Diarize with onnxruntime
-  Segment using pyannote-audio model
-  Transcribe using OpenAI whisper

## Usage

```console
./loud ggml-tiny.bin single.wav --json transcript.json
```

## Building

See [BUILDING.md](docs/BUILDING.md)

