#!/usr/bin/env python3
"""Render the distributed patch's embedded engine using real Pd, without audio hardware."""
import importlib.util
import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('generator', ROOT/'tools/generate_pd.py')
g = importlib.util.module_from_spec(spec)
spec.loader.exec_module(g)
PD = os.environ.get('PD') or shutil.which('pd') or '/Applications/Pd-0.56-2.app/Contents/Resources/bin/pd'


def read_float_wav(path):
    blob = path.read_bytes()
    assert blob[:4] == b'RIFF' and blob[8:12] == b'WAVE'
    pos = 12
    samples = None
    while pos+8 <= len(blob):
        name, size = struct.unpack_from('<4sI',blob,pos)
        data = blob[pos+8:pos+8+size]
        if name == b'fmt ':
            fmt, channels, sr = struct.unpack_from('<HHI',data)
            if fmt == 65534: fmt = struct.unpack_from('<I',data,24)[0]
            assert fmt == 3 and channels == 1, (fmt,channels)
        if name == b'data':
            samples = struct.unpack('<'+'f'*(size//4),data)
        pos += 8 + size + size%2
    assert samples
    return sr,samples


def render(sr, folder):
    p = g.Patch('test')
    load = g.sample_rate(p)
    osc1 = p.obj('osc~ 700',25,25); osc2 = p.obj('osc~ 2200',160,25)
    sum_ = p.obj('+~',25,60);p.wire(osc1,sum_);p.wire(osc2,sum_,inlet=1)
    amplitude = p.obj('line~',250,25); inp = p.obj('*~',25,100)
    p.wire(sum_,inp);p.wire(amplitude,inp,inlet=1)
    engine = p.sub(g.engine(),25,150);p.wire(inp,engine)
    record = p.obj('writesf~ 1',25,200);p.wire(engine,record)
    path = folder/f'render-{sr}.wav'

    def event(ms, text, target=None):
        delay = p.obj(f'delay {ms}')
        msg = p.msg(text)
        p.chain(load,delay,msg)
        if target is not None: p.wire(msg,target)

    event(0,f'open -bytes 4 {path}, start',record)
    event(1,'; pd dsp 1')
    # $0 in message boxes is not canvas-local; route through send objects instead.
    for name,val in [('dry',0),('wet',.55),('enabled',1),('input',8)]:
        target = p.obj(f's $0-{name}')
        event(0,str(val),target)
    # Explicitly connect note messages to the engine's second inlet.
    def note(ms,n,v):
        delay=p.obj(f'delay {ms}');msg=p.msg(f'{n} {v}')
        p.chain(load,delay,msg);p.wire(msg,engine,inlet=1)
    for n in [48,52,55,60]: note(10,n,110)
    event(250,'0.04 5',amplitude)
    event(1000,'0 5',amplitude)
    event(1500,'0.04 5',amplitude)
    for n in [48,52,55,60]: note(1500,n,0)
    for n in [48,52,55,60]: note(2200,n,110)
    enabled=p.obj('s $0-enabled');dry=p.obj('s $0-dry')
    event(2600,'0',enabled)
    event(3000,'0.55',dry)
    event(3400,'0',dry);event(3400,'1',enabled)
    event(3400,'100 5',amplitude)
    event(3800,'0 5',amplitude)
    event(4400,'stop',record)
    event(4450,'; pd quit')
    patch=folder/f'test-{sr}.pd';patch.write_text(p.render()+'\n')
    result=subprocess.run([PD,'-nogui','-noprefs','-noaudio','-nomidi','-stderr','-batch','-r',str(sr),str(patch)],capture_output=True,text=True,timeout=45)
    output=result.stdout+result.stderr
    assert result.returncode==0,output
    assert not any(word in output.lower() for word in ['error','couldn\'t create','connection failed','divide by zero']),output
    rate,samples=read_float_wav(path);assert rate==sr
    assert all(math.isfinite(x) and abs(x)<=.90001 for x in samples)
    def rms(a,b):
        xs=samples[int(a*sr):int(b*sr)]
        return math.sqrt(sum(x*x for x in xs)/len(xs))
    silent=rms(.05,.20);wet=rms(.5,.9);closed=rms(1.35,1.45)
    released=rms(1.85,2.1);restart=rms(2.35,2.5);bypass=rms(2.85,2.95)
    dry_level=rms(3.15,3.3)
    assert silent<1e-6 and closed<1e-5 and released<1e-5 and bypass<1e-5,(silent,closed,released,bypass)
    assert wet>.005 and restart>.005 and dry_level>.01,(wet,restart,dry_level)
    print(f'PASS Pd {sr} Hz: wet RMS={wet:.4f}, dry RMS={dry_level:.4f}, peak={max(map(abs,samples)):.4f}; silence/gate/note-off/bypass/overload')


if __name__ == '__main__':
    assert g.main_patch().render()+'\n' == (ROOT/'pd/WORKING-VOCODER.pd').read_text(), 'Regenerate patch first'
    with tempfile.TemporaryDirectory(prefix='working-vocoder-pd-') as temp:
        for sr in [44100,48000,96000]: render(sr,Path(temp))
