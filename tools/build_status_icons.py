"""Author compact game-style status icons as shared pixel SVG and PNG assets."""
from pathlib import Path
from PIL import Image, ImageDraw
OUT=Path(__file__).resolve().parents[1]/'assets/icons/status_game'
OUT.mkdir(parents=True,exist_ok=True)
colors={'blue':'#54c8f5','cyan':'#aff3ff','red':'#f6653e','yellow':'#ffe36e','green':'#60db79','purple':'#b88ae8','white':'#fff8e8'}
def create(name):
 im=Image.new('RGBA',(16,16));d=ImageDraw.Draw(im)
 if name in ('fire','burn'):
  d.polygon([(7,1),(11,5),(11,8),(13,6),(14,10),(12,13),(9,14),(5,13),(3,10),(4,6),(6,8)],fill=colors['red'])
  d.polygon([(8,6),(11,11),(9,13),(7,13),(6,11)],fill=colors['yellow'])
  if name=='burn':d.point([(2,3),(13,2),(1,9)],fill=colors['yellow'])
 elif name=='water':
  d.polygon([(8,1),(13,8),(13,11),(11,14),(5,14),(3,11),(3,8)],fill=colors['blue'])
  d.line([(5,8),(5,10),(7,12)],fill=colors['white'],width=2)
 elif name=='grass':
  d.polygon([(13,2),(13,8),(10,12),(5,13),(3,10),(4,6),(8,3)],fill=colors['green'])
  d.line([(3,14),(11,5)],fill='#e7ffd2',width=1)
 elif name in ('electric','paralyze'):
  d.polygon([(8,1),(13,1),(10,6),(14,6),(5,15),(7,9),(3,9)],fill=colors['yellow'])
  if name=='paralyze':d.point([(2,3),(13,12),(2,13),(14,3)],fill=colors['red'])
 elif name=='ice':
  for a,b in [((8,2),(8,14)),((2,8),(14,8)),((3,3),(13,13)),((3,13),(13,3))]:d.line([a,b],fill=colors['cyan'],width=2)
  d.point([(5,2),(11,2),(2,5),(2,11),(5,14),(11,14),(14,5),(14,11)],fill=colors['blue'])
 elif name=='freeze':
  d.polygon([(5,2),(12,2),(14,5),(14,12),(11,14),(3,14),(2,11),(2,5)],fill=colors['blue'])
  d.rectangle((4,4,11,11),fill=colors['cyan']);d.line([(5,4),(4,7)],fill=colors['white'],width=2)
 elif name=='slow':
  d.rectangle((3,2,8,9),fill='#d7a45f');d.rectangle((3,8,12,12),fill='#ad7a3d');d.rectangle((2,12,13,13),fill=colors['yellow'])
  d.line([(10,2),(10,6)],fill=colors['cyan'],width=1);d.line([(9,5),(10,6),(11,5)],fill=colors['cyan'],width=1)
 # One-pixel ink outline, preserving the authored pixel-grid geometry.
 base=im.copy()
 for y in range(16):
  for x in range(16):
   if base.getpixel((x,y))[3]:continue
   if any(0<=x+dx<16 and 0<=y+dy<16 and base.getpixel((x+dx,y+dy))[3] for dx,dy in ((1,0),(-1,0),(0,1),(0,-1))):im.putpixel((x,y),(28,46,66,255))
 im.resize((32,32),Image.Resampling.NEAREST).save(OUT/(name+'.png'))
 svg=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" shape-rendering="crispEdges">']
 for y in range(16):
  for x in range(16):
   r,g,b,a=im.getpixel((x,y))
   if a:svg.append(f'<rect x="{x}" y="{y}" width="1" height="1" fill="#{r:02x}{g:02x}{b:02x}"/>')
 svg.append('</svg>');(OUT/(name+'.svg')).write_text(''.join(svg),encoding='utf-8')
for name in ('fire','water','grass','electric','ice','slow','freeze','burn','paralyze'):create(name)
print('Generated 9 status pixel icons (SVG + PNG)')
