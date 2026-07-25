# Gate 1B media fixture

`probe.webm` is a repository-owned test fixture created specifically for
RhythmGame. It is a two-second, 64x64, 30 fps color-motion clip with a 440 Hz
audio tone. The video stream is VP8 (`libvpx`, 160 kbit/s, `yuv420p`) and the
audio stream is Opus (48 kHz, 64 kbit/s constant bitrate). It may be used and
redistributed under the same license as this repository.

Generation command (ffmpeg):

```text
ffmpeg -hide_banner -loglevel error -f lavfi -i "testsrc2=size=64x64:rate=30:duration=2" -f lavfi -i "sine=frequency=440:sample_rate=48000:duration=2" -map 0:v:0 -map 1:a:0 -c:v libvpx -b:v 160k -deadline best -cpu-used 0 -row-mt 0 -pix_fmt yuv420p -c:a libopus -b:a 64k -vbr off -application audio -shortest -map_metadata -1 -fflags +bitexact -flags:v +bitexact -flags:a +bitexact -y probe.webm
```

- Bytes: `58161`
- SHA-256: `654ec3981248ebf738d71dec72c73bfa5fea6861ad0127e3f9807af68b3e45cb`
