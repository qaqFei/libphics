# libphics

一个用于合成Phigros打击音效的轻量C++库。

## 使用方法

导出函数声明:

```C++
#define ulong unsigned long

void process(
    short PCM_CLICK[],  ulong PCM_CLICK_LEN,
    short PCM_FLICK[],  ulong PCM_FLICK_LEN,
    short PCM_DRAG[],   ulong PCM_DRAG_LEN,
    char  chart_data[], ulong note_num,
    short output[],     ulong output_len,
    ulong channels,     ulong frame_rate
);
```

```
PCM_CLICK: tap&hold 音效
PCM_CLICK_LEN: tap&hold 音效的pcm长度 (单位: byte)
PCM_FLICK: flick 音效
PCM_FLICK_LEN: flick 音效的pcm长度 (单位: byte)
PCM_DRAG: drag 音效
PCM_DRAG_LEN: drag 音效的pcm长度 (单位: byte)
chart_data: chart数据 (格式见下文)
note_num: chart数据中的note数量
output: 输出pcm
output_len: 输出pcm长度 (单位: byte)
channels: 声道数
frame_rate: 采样率
```

PCM 均为 16-bit PCM, 每个采样点占用2字节。

chart_data 格式示例:

```python
import struct
data = bytearray()

for i in range(10):
    # 类型: tap=1, drag=2, hold=3, flick=4
    # 时间单位为秒
    data.extend(struct.pack("<Bd", (i % 3) + 1, i * 0.1))
```

调用示例:

```python
import ctypes
import json
import struct
import io
import time

import tqdm
import pydub

lib = ctypes.CDLL("./libphics.dll")
process = lib.process

process.argtypes = [
    ctypes.Array[ctypes.c_short], ctypes.c_ulong,
    ctypes.Array[ctypes.c_short], ctypes.c_ulong,
    ctypes.Array[ctypes.c_short], ctypes.c_ulong,
    ctypes.Array[ctypes.c_char], ctypes.c_ulong,
    ctypes.Array[ctypes.c_short], ctypes.c_ulong,
    ctypes.c_ulong, ctypes.c_ulong
]

process.restype = None

def create_pcm(path: str):
    seg: pydub.AudioSegment = pydub.AudioSegment.from_file(path)
    if seg.channels != 2: seg = seg.set_channels(2)
    if seg.frame_rate != 44100: seg = seg.set_frame_rate(44100)
    if seg.sample_width != 2: seg = seg.set_sample_width(2)
    pcm = bytearray(seg.raw_data)
    length = len(pcm) // 2
    return (ctypes.c_short * length).from_buffer(pcm), length

chart_data = bytearray()
notes = []
for line in json.load(open(input("input chart: "), "r", encoding="utf-8"))["judgeLineList"]:
    for note in line["notesAbove"] + line["notesBelow"]:
        notes.append((note["type"], note["time"] * (1.875 / line["bpm"])))
    
for note in tqdm.tqdm(sorted(notes)):
    chart_data.extend(struct.pack("<Bd", *note))
        
opt_buf, opt_size = create_pcm(input("input music: "))

args = (
    *create_pcm(input("tag&hold: ")),
    *create_pcm(input("flick: ")),
    *create_pcm(input("drag: ")),
    (ctypes.c_char * len(chart_data)).from_buffer(chart_data),
    len(notes),
    opt_buf, opt_size,
    2, 44100
)

st = time.perf_counter()
process(*args)
et = time.perf_counter()
    
print("time:", et - st)
print(len(notes) / (et - st), "notes per second")

output = pydub.AudioSegment.from_raw(
    io.BytesIO(opt_buf),
    sample_width=2,
    frame_rate=44100,
    channels=2
)
output.export(input("output: "))
```
