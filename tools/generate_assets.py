#!/usr/bin/env python3
"""Recreate Skell-Afar's original procedural artwork. Requires Pillow.
Built images are checked into GitHub after repository bootstrap; runtime has no Pillow dependency.
"""
from PIL import Image, ImageDraw, ImageFilter
import math, random
from pathlib import Path
p=Path(__file__).resolve().parents[1] / 'Assets'
p.mkdir(parents=True,exist_ok=True)
rnd=random.Random(78951)
import numpy as np
def dither(arr,rng,amp=1.0):
    return np.clip(arr+rng.uniform(-amp,amp,arr.shape),0,255).astype(np.uint8)
nrng=np.random.default_rng(78951)

# ---- window background: deep neutral charcoal with a faint cool light from the top, vignette and fine grain ----
w,h=1100,700
yy,xx=np.mgrid[0:h,0:w].astype(np.float32)
base=np.zeros((h,w,3),np.float32)
top=np.exp(-(((xx-w*.45)/(w*.62))**2+((yy+40)/(h*.55))**2))          # soft pool of light at the top
vig=np.clip(((xx-w/2)/(w/2))**2*.55+((yy-h/2)/(h/2))**2*.45,0,1)
for i,(c0,c1) in enumerate(((9,20),(12,26),(13,24))): base[...,i]=c0+(c1-c0)*top-(c0*.55)*vig
im=Image.fromarray(dither(base,nrng,1.1),'RGB')
d=ImageDraw.Draw(im,'RGBA')
for k in range(9):                                                    # faint ghost drips from the top edge (brand motif)
    x=int(nrng.integers(30,w-30)); leng=int(nrng.integers(18,60))
    d.line([(x,0),(x+int(nrng.integers(-4,4)),leng)],fill=(70,200,100,int(nrng.integers(10,26))),width=int(nrng.integers(2,5)))
d.line((0,56,w,56),fill=(255,255,255,14),width=1)
im.save(p/'bg_panel.png',optimize=True)

def skull_draw(draw, cx,cy, scale=1, color=(92,255,112,210)):
    f=lambda a:int(a*scale)
    draw.ellipse((cx-f(21),cy-f(25),cx+f(21),cy+f(19)),outline=color,width=max(1,f(3)))
    draw.polygon([(cx-f(14),cy-f(15)),(cx-f(5),cy-f(6)),(cx-f(19),cy-f(6))],fill=color)
    draw.polygon([(cx+f(14),cy-f(15)),(cx+f(5),cy-f(6)),(cx+f(19),cy-f(6))],fill=color)
    draw.line((cx-f(12),cy+f(19),cx-f(12),cy+f(26),cx+f(12),cy+f(26),cx+f(12),cy+f(19)),fill=color,width=max(1,f(3)))
    for tx in [-6,0,6]: draw.line((cx+f(tx),cy+f(19),cx+f(tx),cy+f(26)),fill=color,width=max(1,f(2)))

logo=Image.new('RGBA',(96,96),(0,0,0,0)); ld=ImageDraw.Draw(logo,'RGBA')
ld.ellipse((4,4,92,92),outline=(44,185,60,120),width=3)
skull_draw(ld,48,44,1.32,(110,255,125,250))
logo.save(p/'logo_skull.png')

# ---- 20 scene cards: smooth atmospheric abstractions (no photographs, no logos), tinted by category ----
CW,CH,SSC=660,150,2
TINT={'U':((8,20,24),(30,96,108),(125,222,238)),      # underground: cold teal slate
      'O':((6,14,16),(32,92,70),(150,238,172)),      # outdoor: night green
      'I':((16,15,19),(96,78,62),(244,206,146))}     # indoor: warm slate / amber
CAT=list('UUUUUOOOOOIIIIIIUIOI')   # matches Category in SceneModel.h
def card(idx):
    W,H=CW*SSC,CH*SSC
    lo,hi,glow=TINT[CAT[idx]]
    yy,xx=np.mgrid[0:H,0:W].astype(np.float32); xn=xx/W; yn=yy/H
    gx={0:.78,1:.76,2:.8,3:.75,4:.8,5:.74,6:.7,7:.72,8:.7,9:.78,10:.76,11:.8,12:.76,13:.78,14:.72,15:.78,16:.78,17:.8,18:.72,19:.75}[idx]
    gy={5:.28,6:.32,7:.35,8:.32,18:.3}.get(idx,.5)
    r=np.sqrt(((xn-gx)*1.6)**2+((yn-gy)*.9)**2)
    lit=np.clip(1-r/.75,0,1)**1.6
    arr=np.zeros((H,W,3),np.float32)
    for i in range(3): arr[...,i]=lo[i]+(hi[i]-lo[i])*lit+(1-yn)*5
    layer=Image.new('RGBA',(W,H),(0,0,0,0)); d=ImageDraw.Draw(layer,'RGBA'); g=glow; s=SSC
    X=lambda v:v*W/100.0; Y=lambda v:v*H/100.0
    ln=lambda pts,a,wd=1.4:d.line([(X(x),Y(y)) for x,y in pts],fill=g+(a,),width=max(1,int(wd*s)))
    if idx in (0,2,10,14):                              # one-point perspective runs, each with its own signature
        vx,vy=78,48
        edges=((40,-5),(40,105),(112,-5),(112,105),(40,25),(112,25),(40,75),(112,75))
        if idx==14: edges=((40,-5),(112,-5),(40,105),(112,105))
        for ex,ey in edges: ln([(ex,ey),(vx,vy)],62,1.3)
        for k in range(1,10):
            t=k/10.0; q=t**1.7; f=1-q
            L,R,T,B=vx-(vx-40)*f,vx+(112-vx)*f,vy-(vy+5)*f,vy+(105-vy)*f
            if idx==10:   ln([(L,T),(R,T)],int(80*f)+14,2.2)                       # parking: low ceiling beams
            elif idx==14: ln([(L,T),((L+R)/2,T-8*f),(R,T)],int(80*f)+14,1.6)         # warehouse: roof trusses
            else:         ln([(L,T),(R,T),(R,B),(L,B),(L,T)],int(90*f)+14,1.2)       # tunnel / platform: ribs
            if idx==2 and k%2==0: ln([(L,B-3*f),(R,B-3*f)],int(120*f)+20,1.0)
        if idx==2: d.rectangle((X(40),Y(86),X(112),Y(88)),fill=(250,205,90,90))      # subway: platform edge strip
        if idx==10: [ln([(x,0),(x,100)],40,2.6) for x in (58,66,72)]                   # garage pillars
        d.ellipse((X(vx-3),Y(vy-6),X(vx+3),Y(vy+6)),fill=g+(190,))
    elif idx in (1,19):                                  # round pipe / bowl: concentric ellipses
        for k in range(1,12): rr=k*k*.42+1; d.ellipse((X(78-rr*1.5),Y(50-rr*.7),X(78+rr*1.5),Y(50+rr*.7)),outline=g+(int(130-k*7),),width=int(1.6*s))
    elif idx==3:                                          # cave: stalactites and stalagmites
        rng=np.random.default_rng(3)
        for k in range(14):
            x=45+k*4.4+rng.uniform(-2,2); hgt=rng.uniform(12,38); wd=rng.uniform(1.4,3.2)
            d.polygon([(X(x-wd),Y(-2)),(X(x+wd),Y(-2)),(X(x),Y(hgt))],fill=g+(46,))
            hgt=rng.uniform(8,30); d.polygon([(X(x+2-wd),Y(102)),(X(x+2+wd),Y(102)),(X(x+2),Y(100-hgt))],fill=g+(38,))
    elif idx==4:                                          # cellar: small box room with a bulb
        ln([(52,18),(96,18),(96,92),(52,92),(52,18)],60,1.5); ln([(60,28),(88,28),(88,82),(60,82),(60,28)],40,1.2)
        for a,b in (((52,18),(60,28)),((96,18),(88,28)),((96,92),(88,82)),((52,92),(60,82))): ln([a,b],34,1.1)
        d.ellipse((X(73),Y(14),X(77),Y(26)),fill=g+(190,))
    elif idx in (5,16):                                   # graveyard stones / crypt arches
        base=82 if idx==5 else 100
        for k in range(9):
            x=52+k*5.4+(k%3)*.8; wd=2.0 if idx==5 else 2.6; hh=(14+(k*7)%10) if idx==5 else 40
            if idx==5:
                d.rounded_rectangle((X(x-wd),Y(base-hh),X(x+wd),Y(base+4)),radius=int(X(wd)),fill=(2,8,8,170),outline=g+(60,),width=int(1.2*s))
            else:
                d.arc((X(x-wd*2),Y(30),X(x+wd*2),Y(70)),180,360,fill=g+(90,),width=int(1.6*s)); ln([(x-wd*2,50),(x-wd*2,100)],70,1.6); ln([(x+wd*2,50),(x+wd*2,100)],70,1.6)
        if idx==5: d.ellipse((X(90),Y(10),X(90)+Y(22),Y(32)),fill=g+(120,))   # moon, on the art side of the card
    elif idx==6:                                          # forest: tapering trunks in fog
        rng=np.random.default_rng(6)
        for k in range(16):
            x=44+k*3.9+rng.uniform(-1.5,1.5); wd=rng.uniform(.5,1.3)
            d.polygon([(X(x-wd),Y(-2)),(X(x+wd),Y(-2)),(X(x+wd*1.5),Y(102)),(X(x-wd*1.5),Y(102))],fill=(2,10,8,int(rng.integers(70,150))))
    elif idx==7:                                          # canyon: stacked ridges
        rng=np.random.default_rng(7)
        for layer_i,(yb,al) in enumerate(((62,38),(74,56),(88,84))):
            pts=[(40,100)]+[(40+k*3.2,yb+rng.uniform(-10,8)-layer_i*2) for k in range(0,21)]+[(112,100)]
            d.polygon([(X(a),Y(b)) for a,b in pts],fill=(2,10,10,al),outline=g+(40,))
    elif idx in (8,18):                                   # open field / snowfield: horizon and gentle swell
        pts=[(40,66)]+[(40+k*3.6,64+3*np.sin(k*.55)) for k in range(0,21)]+[(112,100),(40,100)]
        d.polygon([(X(a),Y(b)) for a,b in pts],fill=(2,9,10,100),outline=g+(70,))
        if idx==18:
            rng=np.random.default_rng(18)
            for k in range(60): fx,fy=rng.uniform(40,100),rng.uniform(4,95); d.ellipse((X(fx),Y(fy),X(fx)+3*s,Y(fy)+3*s),fill=g+(int(rng.integers(50,140)),))
    elif idx==9:                                          # city alley: facing walls with lit windows
        for k in range(8):
            x=46+k*7; hh=36+(k*13)%34; d.rectangle((X(x),Y(100-hh),X(x+6),Y(100)),fill=(2,8,10,150),outline=g+(40,))
            for wy in range(int(100-hh+6),96,9):
                if (k*7+wy)%3: d.rectangle((X(x+1.5),Y(wy),X(x+3.2),Y(wy+3.2)),fill=g+(90,))
    elif idx==11:                                         # stairwell: climbing flights
        for k in range(10): x=44+k*6.2; ln([(x,92-k*7),(x+6.2,92-k*7),(x+6.2,92-(k+1)*7)],62,1.5)
        ln([(44,98),(106,24)],34,1.0)
    elif idx==12:                                         # cathedral: pointed arches and a light shaft
        for k,xc in enumerate((56,70,84,98)):
            d.arc((X(xc-7),Y(8),X(xc+7),Y(8)+X(30)),180,360,fill=g+(80,),width=int(1.6*s)); ln([(xc-7,28),(xc-7,100)],60,1.5); ln([(xc+7,28),(xc+7,100)],60,1.5)
        d.polygon([(X(62),Y(0)),(X(70),Y(0)),(X(92),Y(100)),(X(72),Y(100))],fill=g+(20,))
    elif idx==13:                                         # empty pool: tiled basin in perspective
        for k in range(0,12): ln([(44+k*6,100),(70+k*1.5,40)],42,1.0)
        for k in range(1,8): yq=40+k*k*1.2; ln([(44-k*1.2,yq),(108+k*1.2,yq)],44,1.0)
    elif idx==15:                                         # bathroom: flat tile grid with a soft sheen
        for gx in range(44,110,5): ln([(gx,0),(gx,100)],36,1.0)
        for gy in range(0,101,12): ln([(44,gy),(110,gy)],36,1.0)
    elif idx==17:                                         # elevator shaft: tall rails and rungs
        for gx in (56,66,76,86): ln([(gx,-2),(gx,102)],80,1.6)
        for gy in range(8,100,13): ln([(56,gy),(86,gy)],34,1.0)
    soft=layer.filter(ImageFilter.GaussianBlur(.5*SSC)); glowl=layer.filter(ImageFilter.GaussianBlur(5*SSC))
    base_img=Image.fromarray(np.clip(arr,0,255).astype(np.uint8),'RGB').convert('RGBA')
    base_img.alpha_composite(glowl); base_img.alpha_composite(soft)
    out=np.asarray(base_img.convert('RGB').resize((CW,CH),Image.LANCZOS),np.float32)
    yv,xv=np.mgrid[0:CH,0:CW].astype(np.float32)
    vg=1-np.clip((((xv/CW)-.5)/.5)**2*.35+(((yv/CH)-.5)/.5)**2*.25,0,1)*.9
    out=out*vg[...,None]
    return Image.fromarray(dither(out,nrng,.9),'RGB')
for idx in range(20): card(idx).save(p/f'space_{idx:02d}.png',optimize=True)
