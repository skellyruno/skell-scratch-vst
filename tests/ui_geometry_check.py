"""Static editor bounds regression check. Not a rendered GUI/DAW smoke test."""
import pathlib
import re

editor = (pathlib.Path(__file__).parent.parent / 'Source' / 'PluginEditor.cpp').read_text(encoding='utf-8')
pattern = re.compile(r'\b(curve|knob|stop|lock|footer)\.setBounds\(bounds\((\d+),(\d+),(\d+),(\d+)\)\);')
logical = {m[1]: tuple(int(x) for x in m.groups()[1:]) for m in pattern.finditer(editor)}
assert set(logical) == {'curve', 'knob', 'stop', 'lock', 'footer'}, logical
assert 'constrainer.setFixedAspectRatio(441.0/263.0);' in editor
assert 'constrainer.setSizeLimits(441,263,1323,789);' in editor
for width, height in ((441,263),(661,394),(882,526),(1323,789)):
    scaled = {}
    for name, (x, y, w, h) in logical.items():
        rect = tuple(round(v * scale) for v, scale in zip((x,y,w,h),(width/441,height/263,width/441,height/263)))
        scaled[name] = rect
        xx, yy, ww, hh = rect
        assert min(xx,yy)>=0 and min(ww,hh)>0
        assert xx+ww<=width and yy+hh<=height, (name,rect,width,height)
    names=list(scaled)
    for ai,a in enumerate(names):
        ax,ay,aw,ah=scaled[a]
        for b in names[ai+1:]:
            bx,by,bw,bh=scaled[b]
            assert not (ax<bx+bw and ax+aw>bx and ay<by+bh and ay+ah>by), (a,b,scaled)
    assert scaled['stop'][3]>=38 and scaled['lock'][3]>=25
    print(f'{width}x{height}: all 5 controls inside bounds, no overlaps; lock/stop have usable hit heights')
