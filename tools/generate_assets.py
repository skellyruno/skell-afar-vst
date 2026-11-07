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
w,h=1100,700
im=Image.new('RGB',(w,h)); pix=im.load()
for y in range(h):
    for x in range(w):
        vig=(abs(x-w/2)/(w/2))**1.3*.11+(abs(y-h/2)/(h/2))**1.2*.08
        noise=rnd.randrange(-3,4)
        pix[x,y]=(max(1,int(8+noise-8*vig)),max(1,int(12+noise-8*vig)),max(1,int(13+noise-7*vig)))
d=ImageDraw.Draw(im,'RGBA')
for k in range(22):
    x=rnd.randrange(10,w-10); leng=rnd.randrange(14,95)
    d.line([(x,0),(x+rnd.randrange(-10,10),leng)],fill=(39,180,45,rnd.randrange(25,80)),width=rnd.randrange(2,9))
d.line((0,56,w,56),fill=(62,120,70,120),width=1)
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
# 16 original scene cards, abstract geometric silhouettes, no photographic assets.
palettes=[(24,62,54),(19,66,69),(36,61,75),(37,61,54),(34,47,51),(21,46,45),(23,49,38),(62,45,43),(29,50,48),(44,54,49),(35,50,57),(30,44,55),(52,51,56),(35,60,61),(47,52,54),(30,62,39)]
for idx in range(16):
    img=Image.new('RGB',(480,200)); px=img.load()
    base=palettes[idx]
    for y in range(200):
        for x in range(480):
            shift=math.sin(x/67.0+idx)*6+math.cos(y/59.0)*5
            grit=rnd.randint(-7,7)
            px[x,y]=tuple(max(0,min(255,int(c+shift+grit-(x/480)*11))) for c in base)
    di=ImageDraw.Draw(img,'RGBA')
    if idx in (0,1,2,4,9,10,11,12,13,14):
        for j in range(2,11):
            offs=j*j*2.1
            di.rounded_rectangle((int(offs),int(offs*.45),480-int(offs),200-int(offs*.45)),radius=7,outline=(110,195,130,65),width=2)
    elif idx in (5,6):
        for k in range(17):
            x=rnd.randrange(18,470); y=rnd.randrange(60,200)
            di.line((x,y,x+rnd.randrange(-7,7),y-40),fill=(90,170,110,70),width=6)
            di.ellipse((x-18,y-55,x+18,y-20),outline=(100,195,105,80),width=3)
    elif idx==7:
        for k in range(5):
            di.polygon([(k*130,180),(k*130+80,35),(k*130+155,180)],outline=(240,155,105,70))
    elif idx==8:
        di.line((0,150,480,145),fill=(190,230,190,65),width=2)
    if idx!=15:
        skull_draw(di,402,102,1.08,(106,253,123,105))
    else:
        skull_draw(di,240,95,1.3,(106,253,123,155))
    img.save(p/f'space_{idx:02d}.png',optimize=True)
