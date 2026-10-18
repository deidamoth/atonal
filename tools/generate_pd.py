#!/usr/bin/env python3
"""Generate a standalone Vanilla Pd patch; all DSP subpatches are embedded."""
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def esc(s):
    return re.sub(r'\$(\d)', r'\\$\1', s).replace(',', r' \, ').replace(';', r' \; ')


class Patch:
    def __init__(self, name='patch', width=1100, height=760):
        self.name, self.width, self.height = name, width, height
        self.items, self.connections = [], []

    def add(self, kind, text, x=25, y=None):
        i = len(self.items)
        if y is None:
            y = 30 + i * 32
        self.items.append(f'#X {kind} {x} {y} {esc(text)};')
        return i

    def obj(self, text, x=25, y=None):
        return self.add('obj', text, x, y)

    def msg(self, text, x=25, y=None):
        return self.add('msg', text, x, y)

    def text(self, text, x=25, y=None):
        return self.add('text', text, x, y)

    def sub(self, patch, x=25, y=100):
        i = len(self.items)
        self.items.append(patch.render(False) + f'\n#X restore {x} {y} pd {patch.name};')
        return i

    def wire(self, src, dst, outlet=0, inlet=0):
        self.connections.append(f'#X connect {src} {outlet} {dst} {inlet};')

    def chain(self, *nodes):
        for a, b in zip(nodes, nodes[1:]):
            self.wire(a, b)
        return nodes[-1]

    def render(self, root=True):
        suffix = '12' if root else f'{self.name} 0'
        return '\n'.join([f'#N canvas 50 50 {self.width} {self.height} {suffix};',
                          *self.items, *self.connections])


def follower(p, source, attack, x=25, y=200):
    env = p.obj('fexpr~ $y1[-1] + if(abs($x1[0]) > $y1[-1], $f2, $f3) * (abs($x1[0]) - $y1[-1])', x, y)
    sr = p.obj('r $0-sr', x, y+35)
    a = p.obj(f'expr 1-exp(-1/({attack:.10g}*$f1))', x, y+70)
    r = p.obj('expr 1-exp(-1/(0.035*$f1))', x+320, y+70)
    p.wire(source, env); p.chain(sr, a); p.chain(sr, r)
    p.wire(a, env, inlet=1); p.wire(r, env, inlet=2)
    return env


def voice(number):
    p = Patch(f'voice-{number}', 1100, 650)
    incoming = p.obj('inlet', 25, 25)
    unpack = p.obj('unpack f f', 25, 65)
    hz = p.obj('mtof', 25, 105)
    phase = p.obj('phasor~', 25, 145)
    sr = p.obj('r $0-sr', 300, 25)
    divide = p.obj('/ 48000', 180, 145)
    # Store frequency for recomputation if the audio sample rate changes.
    store = p.obj('f', 180, 105)
    trig = p.obj('t b f', 300, 65)
    p.wire(sr, trig); p.wire(trig, divide, outlet=1, inlet=1); p.wire(trig, store)
    p.chain(incoming, unpack, hz, phase)
    p.wire(hz, store); p.wire(store, divide)
    saw = p.obj('expr~ 2*$v1-1-if($v1<$f2, 2*$v1/$f2-pow($v1/$f2,2)-1, if($v1>1-$f2, pow(($v1-1)/$f2,2)+2*($v1-1)/$f2+1, 0))', 25, 205)
    dt = p.obj('max 0.000001', 180, 175)
    p.chain(divide, dt); p.wire(dt, saw, inlet=1); p.wire(phase, saw)
    select = p.obj('sel 0', 500, 105)
    off = p.msg('0 100', 500, 145)
    vel = p.obj('expr 0.15+0.25*$f1/127', 630, 145)
    on = p.msg('$1 5', 630, 180)
    envelope = p.obj('vline~', 500, 250)
    p.wire(unpack, select, outlet=1); p.chain(select, off, envelope)
    p.wire(select, vel, outlet=1); p.chain(vel, on, envelope)
    mul = p.obj('*~', 25, 300); out = p.obj('outlet~', 25, 340)
    p.chain(saw, mul, out); p.wire(envelope, mul, inlet=1)
    p.text('PolyBLEP saw; velocity amplitude; 5 ms attack / 100 ms release.', 25, 400)
    return p


def channel(b):
    hz = 120 * (7500/120)**(b/19)
    p = Patch(f'band-{b+1}-{round(hz)}Hz', 1100, 650)
    mod = p.obj('inlet~', 25, 25); car = p.obj('inlet~', 600, 25)
    sr = p.obj('r $0-sr', 300, 25)
    omega = p.obj(f'expr 6.28318530718*{hz:.10g}/$f1', 300, 65)
    coeff = p.obj('expr 2*cos($f1)/(1+sin($f1)/7); -(1-sin($f1)/7)/(1+sin($f1)/7); (sin($f1)/7)/(1+sin($f1)/7); 0; -(sin($f1)/7)/(1+sin($f1)/7)', 300, 105)
    pack = p.obj('pack f f f f f', 300, 145)
    p.chain(sr, omega, coeff)
    for o in range(5): p.wire(coeff, pack, o, o)
    filters = [p.obj('biquad~', x, y) for x,y in [(25,185),(25,225),(600,185),(600,225)]]
    for f in filters: p.wire(pack, f)
    p.chain(mod, *filters[:2]); p.chain(car, *filters[2:])
    env = follower(p, filters[1], max(.002,1/hz), 25, 290)
    mul = p.obj('*~', 600, 425); out = p.obj('outlet~', 600, 470)
    p.chain(filters[3], mul, out); p.wire(env, mul, inlet=1)
    return p


def smoothed_control(p, name, x, y):
    r = p.obj(f'r $0-{name}', x, y)
    clip = p.obj('clip 0 1', x, y+30)
    sig = p.obj('sig~', x, y+60)
    smooth = p.obj('fexpr~ $y1[-1]+$f2*($x1[0]-$y1[-1])', x, y+90)
    coef = p.obj('r $0-smooth', x+300, y+60)
    p.chain(r, clip, sig, smooth); p.wire(coef, smooth, inlet=1)
    return smooth


def engine():
    p = Patch('vocoder-engine', 1400, 1000)
    inp = p.obj('inlet~', 25, 25); notes = p.obj('inlet', 700, 25)
    poly = p.obj('poly 4 1', 700, 65); pack = p.obj('pack f f f', 700, 105)
    route = p.obj('route 1 2 3 4', 700, 145)
    p.wire(notes, poly)
    for o in range(3): p.wire(poly, pack, o, o)
    p.wire(pack, route)
    panic = p.obj('r $0-panic', 1000, 25); stop = p.msg('stop', 1000, 65)
    p.chain(panic, stop, poly)
    summed = p.obj('*~ 0.5', 700, 240)
    for v in range(4):
        node = p.sub(voice(v+1), 700+v*150, 190)
        p.wire(route, node, outlet=v); p.wire(node, summed)
    bright = p.obj('fexpr~ 4*($x1[0]-0.96*$x1[-1])', 700, 280)
    p.wire(summed, bright)
    dc = p.obj('fexpr~ $x1[0]-$x1[-1]+$f2*$y1[-1]', 25, 70)
    sr = p.obj('r $0-sr', 380, 25)
    dc_coef = p.obj('expr exp(-6.28318530718*70/$f1)', 380, 65)
    p.chain(sr, dc_coef); p.wire(dc_coef, dc, inlet=1); p.wire(inp, dc)
    gain = p.obj('r $0-input', 25, 115); gainmsg = p.obj('pack f 10', 25, 150)
    gainline = p.obj('line~', 25, 185); mic = p.obj('*~', 25, 225)
    p.chain(gain, gainmsg, gainline); p.wire(gainline, mic, inlet=1); p.wire(dc, mic)
    speech = follower(p, mic, .002, 25, 270)
    gatecurve = p.obj('expr~ min(1,max(0,($v1-0.004)/0.016))', 25, 390)
    smoothstep = p.obj('expr~ $v1*$v1*(3-2*$v1)', 25, 425)
    gate = p.obj('fexpr~ $y1[-1]+$f2*($x1[0]-$y1[-1])', 25, 460)
    p.chain(speech, gatecurve, smoothstep, gate)
    coef = p.obj('r $0-smooth', 380, 425); p.wire(coef, gate, inlet=1)
    wet_sum = p.obj('*~ 16', 700, 540)
    for b in range(20):
        node = p.sub(channel(b), 650+(b%5)*140, 340+(b//5)*45)
        p.wire(mic, node); p.wire(bright, node, inlet=1); p.wire(node, wet_sum)
    gated = p.obj('*~', 700, 580); p.chain(wet_sum, gated); p.wire(gate, gated, inlet=1)
    dry = smoothed_control(p, 'dry', 25, 560)
    wet = smoothed_control(p, 'wet', 700, 650)
    enabled = smoothed_control(p, 'enabled', 25, 745)
    drymul = p.obj('*~', 450, 690); wetmul = p.obj('*~', 700, 790)
    enablemul = p.obj('*~', 700, 830); mix = p.obj('+~', 450, 870)
    p.wire(mic, drymul); p.wire(dry, drymul, inlet=1)
    p.wire(gated, wetmul); p.wire(wet, wetmul, inlet=1)
    p.chain(wetmul, enablemul); p.wire(enabled, enablemul, inlet=1)
    p.wire(drymul, mix); p.wire(enablemul, mix, inlet=1)
    limit = p.obj('expr~ 0.9*$v1/(0.9+abs($v1))', 450, 910)
    out = p.obj('outlet~', 450, 950); p.chain(mix, limit, out)
    return p


def sample_rate(p, x=800, y=530):
    lb = p.obj('loadbang', x, y)
    pd = p.obj('r pd', x+150, y); route = p.obj('route dsp', x+150, y+30)
    sel = p.obj('sel 1', x+150, y+60); delay = p.obj('delay 0', x, y+90)
    sr = p.obj('samplerate~', x, y+120); minimum = p.obj('max 22050', x, y+150)
    send = p.obj('s $0-sr', x, y+185)
    coef = p.obj('expr 1-exp(-1/(0.010*$f1))', x+150, y+185)
    smooth = p.obj('s $0-smooth', x+150, y+220)
    p.chain(pd, route, sel, delay); p.wire(lb, delay)
    p.chain(delay, sr, minimum, send); p.chain(minimum, coef, smooth)
    return lb


def main_patch():
    p = Patch('WORKING-VOCODER', 1100, 820)
    p.text('WORKING VOCODER | 4 MIDI voices / 20 speech bands', 25, 15)
    p.text('1. Select audio + MIDI devices in Preferences/Settings. Use headphones.',25,45)
    p.text('2. Enable DSP. Hold a chord and speak. Dry=0 gives vocoder only.',25,70)
    p.text('Self-contained Pure Data Vanilla / plugdata patch. No Daisy required.',25,95)
    dsp = p.obj('tgl 24 0 empty empty DSP_audio 30 12 0 12 #dfdfdf #000000 #000000 0 1',25,140)
    dspmsg = p.msg('; pd dsp $1',25,180); p.wire(dsp,dspmsg)
    enabled = p.obj('tgl 24 0 $0-enabled empty Vocoder_on 30 12 0 12 #dfdfdf #000000 #000000 0 1',250,140)
    dry = p.obj('hsl 240 22 0 0.55 0 0 $0-dry empty Dry_voice 0 -10 0 12 #dfdfdf #000000 #000000 0 1',25,245)
    wet = p.obj('hsl 240 22 0 0.55 0 0 $0-wet empty Wet_vocoder 0 -10 0 12 #dfdfdf #000000 #000000 0 1',320,245)
    gain = p.obj('hsl 240 22 0 16 0 0 $0-input empty Input_gain_0_to_16 0 -10 0 12 #dfdfdf #000000 #000000 0 1',25,315)
    blend = p.obj('hsl 240 22 0 1 0 0 empty empty Optional_mix_left_WET_right_DRY 0 -10 0 12 #dfdfdf #000000 #000000 0 1',320,315)
    blenddry = p.obj('* 0.55',320,355); blendwet = p.obj('expr 0.55*(1-$f1)',450,355)
    p.wire(blend,blenddry); p.wire(blend,blendwet); p.wire(blenddry,dry); p.wire(blendwet,wet)
    panic = p.obj('bng 24 250 50 0 $0-panic empty Panic_notes_off 30 12 0 12 #dfdfdf #000000 #000000',650,140)
    adc = p.obj('adc~ 1 2',25,430)
    source = p.obj('hradio 20 1 0 3 empty empty Input:_1__2__average 0 -10 0 12 #dfdfdf #000000 #000000 0',650,245)
    select = p.obj('expr~ if($f3==0,$v1,if($f3==1,$v2,($v1+$v2)*0.5))',25,475)
    p.wire(adc,select);p.wire(adc,select,1,1);p.wire(source,select,inlet=2)
    notein = p.obj('notein',650,430); pack = p.obj('pack f f',650,475)
    p.wire(notein,pack);p.wire(notein,pack,1,1)
    eng = p.sub(engine(),25,530);p.wire(select,eng);p.wire(pack,eng,inlet=1)
    dac = p.obj('dac~ 1 2',25,580);p.wire(eng,dac);p.wire(eng,dac,inlet=1)
    lb = sample_rate(p,650,530)
    for val,target,x in [(0,dry,25),(.55,wet,125),(8,gain,225),(1,enabled,325),(0,source,425)]:
        msg=p.msg(str(val),x,660);p.wire(lb,msg);p.wire(msg,target)
    p.text('Dry / wet are independent. Optional mix slider sets both as a crossfade.',25,710)
    p.text('Input defaults to channel 1. No output until DSP is enabled. See pd/README.md.',25,740)
    p.text('Same filter / gate design as hardware; Pd voice ramps and scheduling differ.',25,770)
    return p


if __name__ == '__main__':
    (ROOT/'pd'/'WORKING-VOCODER.pd').write_text(main_patch().render()+'\n')
